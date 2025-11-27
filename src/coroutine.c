#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
// macOS uses MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif

#include "array.h"
#include "coroutine.h"

/* Private Types */

struct s_ctx {
  void *rsp;
  void *stack_base;
  bool is_done;
  size_t stack_size;
};

struct s_coroutines {
  da_struct(sp_ctx);
};

struct s_stack {
  // Dynamic array of coroutine contexts
  struct s_coroutines active_ctxs;
  struct s_coroutines inactive_ctxs;

  // Current context index
  size_t current_index;
  size_t stack_size;
};

// Global contexts stack
// struct s_stack g_ctx = {};

#define INVALID_CTX_ID (~((size_t)0))

// Current coroutine context (NULL for main context)
// size_t current_ctx_id = INVALID_CTX_ID;

/* Platform Specific Functions */

/**
 * @brief restore context from rsp
 * @param rsp The stack pointer to restore from
 */
void _asm_restore_ctx(void *);

/**
 * @brief Switch to the given coroutine context
 * @param ctx The coroutine context to switch to
 */
void switch_ctx(sp_stack, sp_ctx);

/**
 * @brief Yield to the previous coroutine context
 */
void yield_ctx(sp_stack);

void *platform_setup_stack(void *, sp_func, sp_stack, void *);

/* Private Functions */

void coroutine_finish(sp_stack stack) {
  size_t current_ctx_id = stack->current_index;
  assert(current_ctx_id != 0 && "Main context cannot finish");

  sp_ctx current_ctx = stack->active_ctxs.items[current_ctx_id];
  current_ctx->is_done = true; // mark as done

  da_fast_remove(&stack->active_ctxs, current_ctx_id);

  sp_ctx ctx = stack->active_ctxs.items[--stack->current_index];
  _asm_restore_ctx(ctx->rsp);

  // Unreachable code here
  assert(false && "coroutine_finish: Unreachable code reached");
  abort();
}

size_t get_ctx_id(sp_stack stack) { return stack->current_index; }

size_t get_ctx_id_of(sp_stack stack, sp_ctx ctx) {
  if (ctx == NULL)
    return 0; // Main context

  for (size_t i = 0; i < stack->active_ctxs.count; i++) {
    if (stack->active_ctxs.items[i] == ctx) {
      return i;
    }
  }

  return INVALID_CTX_ID;
}

__attribute__((noreturn)) void switch_ctx_inner(sp_stack stack, sp_ctx ctx,
                                                void *rsp) {
  if (ctx == NULL) {
    ctx = stack->active_ctxs.items[0]; // Main context
  }

  sp_ctx current_ctx = stack->active_ctxs.items[stack->current_index];
  // Save current rsp
  current_ctx->rsp = rsp;

  size_t new_ctx_idx = get_ctx_id_of(stack, ctx);
  assert(new_ctx_idx != INVALID_CTX_ID && "Target context not found");
  stack->current_index = new_ctx_idx;

  // Switch contexts
  _asm_restore_ctx(ctx->rsp);

  abort(); // Unreachable
}

__attribute__((noreturn)) void yield_ctx_inner(sp_stack stack, void *rsp) {

  sp_ctx current_ctx = stack->active_ctxs.items[stack->current_index];
  // Save current rsp
  current_ctx->rsp = rsp;

  if (stack->current_index == 0) {
    stack->current_index =
        stack->active_ctxs.count - 1; // Switch to last context
  } else {
    stack->current_index--; // Switch to previous context
  }

  sp_ctx ctx = stack->active_ctxs.items[stack->current_index];

  // Switch contexts
  _asm_restore_ctx(ctx->rsp);

  abort(); // Unreachable
}

/* Public Functions */

sp_stack init_stack(size_t stack_capacity) {
  if (stack_capacity == 0)
    stack_capacity = STACK_CAPACITY;

  sp_stack stack = malloc(sizeof(*stack));
  da_init(&stack->active_ctxs);

  stack->current_index = 0;
  stack->stack_size = stack_capacity;

  // Setup main context (caller thread)
  sp_ctx ctx = malloc(sizeof(*ctx));
  ctx->rsp = NULL;
  ctx->stack_base = NULL;

  da_append(&stack->active_ctxs, ctx);

  return stack;
}

void deinit_stack(sp_stack stack) {
  assert(stack->active_ctxs.count == 1 &&
         "All coroutines must be destroyed before deinitializing the stack");

  free(stack->active_ctxs
           .items[0]); // Destroy main context (only free as no mmap was used)

  da_free(&stack->active_ctxs);
  free(stack);
}

/**
 * @brief Create a new coroutine context
 *
 * @param fn The coroutine function
 * @param arg The argument to the coroutine function
 *
 * @return sp_ctx The created coroutine context
 */
sp_ctx create_ctx(sp_stack stack, sp_func fn, void *arg) {
  sp_ctx ctx = malloc(sizeof(*ctx));
  ctx->stack_size = stack->stack_size;

  int prot = PROT_WRITE | PROT_READ;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_STACK
  flags |= MAP_STACK;
#endif
#ifdef MAP_GROWSDOWN
  flags |= MAP_GROWSDOWN;
#endif

  ctx->stack_base = mmap(NULL, ctx->stack_size, prot, flags, -1, 0);
  assert(ctx->stack_base != MAP_FAILED &&
         "Failed to allocate stack for coroutine");

  ctx->rsp = platform_setup_stack((char *)ctx->stack_base + stack->stack_size,
                                  fn, stack, arg);
  ctx->is_done = false;

  da_append(&stack->active_ctxs, ctx);

  return ctx;
}

void unregister_ctx(sp_stack stack, sp_ctx ctx) {
  assert(ctx != get_ctx(stack) && "Cannot unregister main or current context");

  size_t idx = get_ctx_id_of(stack, ctx);
  if (idx != INVALID_CTX_ID) {
    da_fast_remove(&stack->active_ctxs, idx);
    if (stack->current_index >= idx && stack->current_index > 0) {
      stack->current_index--;
    }
  }
}

/**
 * @brief Destroy a coroutine context
 *
 * @param ctx The coroutine context to destroy
 *
 * @warning Cannot destroy the main context or a non-finished context
 */
void destroy_ctx(sp_ctx ctx) {
  assert(ctx != NULL && "Cannot destroy main context");
  assert(ctx->is_done && "Cannot destroy a non-finished context");

  munmap(ctx->stack_base, ctx->stack_size);
  free(ctx);
}

bool is_ctx_finished(sp_ctx ctx) {
  if (ctx == NULL)
    return false; // Main context is never finished

  return ctx->is_done;
}

sp_ctx get_ctx(sp_stack stack) {
  size_t idx = get_ctx_id(stack);
  if (idx == 0)
    return NULL; // Main context

  assert(idx != INVALID_CTX_ID && "No current context");
  return stack->active_ctxs.items[idx];
}
