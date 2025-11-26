#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>

#ifndef MAP_ANONYMOUS
// macOS uses MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif

#include "array.h"
#include "coroutine.h"

struct s_ctx{
    void *rsp;
    void *stack_base;
    bool is_done;
};

typedef struct {
    da_struct(sp_ctx);
} da_ctx;


// Global contexts stack
da_ctx g_ctx = {};

#define INVALID_CTX_IDX (~((size_t)0))

// Current coroutine context (NULL for main context)
size_t current_ctx_idx = INVALID_CTX_IDX;

/**
 * @brief restore context from rsp
 * @param rsp The stack pointer to restore from
 */
void _asm_restore_ctx(void *);

/**
 * @brief Switch to the given coroutine context
 * @param ctx The coroutine context to switch to
 */
void switch_ctx(sp_ctx);

/**
 * @brief Yield to the previous coroutine context
 */
void yield_ctx(void);

void coroutine_finish(void) {
    assert(current_ctx_idx != 0 && "Main context cannot finish");
    sp_ctx current_ctx = g_ctx.items[current_ctx_idx];
    current_ctx->is_done = true; // mark as done

    da_fast_remove(&g_ctx, current_ctx_idx);

    sp_ctx ctx = g_ctx.items[--current_ctx_idx];
    _asm_restore_ctx(ctx->rsp);
}

void* platform_setup_stack(void*, coroutine, finalizer, void*);

/**
 * @brief Create a new coroutine context
 *
 * @param fn The coroutine function
 * @param arg The argument to the coroutine function
 *
 * @return sp_ctx The created coroutine context
 */
sp_ctx create_ctx(coroutine fn, void* arg) {

    if (current_ctx_idx == (size_t)(-1)) {
        // Initialize main context
        current_ctx_idx = 0;
        sp_ctx main_ctx = malloc(sizeof(*main_ctx));
        main_ctx->rsp = NULL;
        main_ctx->stack_base = NULL;
        main_ctx->is_done = false;
        da_append(&g_ctx, main_ctx);
    }

    sp_ctx ctx = malloc(sizeof(*ctx));

    int prot = PROT_WRITE | PROT_READ;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_STACK
    flags |= MAP_STACK;
#endif
#ifdef MAP_GROWSDOWN
    flags |= MAP_GROWSDOWN;
#endif

    ctx->stack_base =  mmap(NULL, STACK_CAPACITY, prot, flags, -1, 0);
    assert(ctx->stack_base != MAP_FAILED && "Failed to allocate stack for coroutine");

    ctx->rsp = platform_setup_stack(ctx->stack_base + STACK_CAPACITY, fn, coroutine_finish, arg);
    ctx->is_done = false;

    da_append(&g_ctx, ctx);

    return ctx;
}

/**
 * @brief Destroy a coroutine context
 *
 * @param ctx The coroutine context to destroy
 */
void destroy_ctx(sp_ctx ctx) {
    assert(ctx != NULL && "Cannot destroy main context");

    munmap(ctx->stack_base, STACK_CAPACITY);
    free(ctx);
}

bool is_ctx_finished(sp_ctx ctx) {
    if (ctx == NULL) return false; // Main context is never finished

    return ctx->is_done;
}

size_t get_ctx_id(void) {
    return current_ctx_idx;
}

sp_ctx get_ctx_ptr(void) {
    size_t idx = get_ctx_id();
    if (idx == 0) return NULL; // Main context

    assert(idx != INVALID_CTX_IDX && "No current context");
    return g_ctx.items[idx];
}

size_t get_idx_of(sp_ctx ctx) {
    if (ctx == NULL) return 0; // Main context


    for (size_t i = 0; i < g_ctx.count; i++) {
        if (g_ctx.items[i] == ctx) {
            return i;
        }
    }

    return INVALID_CTX_IDX;
}

void switch_ctx_inner(void* curent_rsp, sp_ctx ctx) {
    if (ctx == NULL) {
        ctx = g_ctx.items[0]; // Main context
    }

    sp_ctx current_ctx = g_ctx.items[current_ctx_idx];
    // Save current rsp
    current_ctx->rsp = curent_rsp;

    size_t new_ctx_idx = get_idx_of(ctx);
    assert(new_ctx_idx != INVALID_CTX_IDX && "Target context not found");
    current_ctx_idx = new_ctx_idx;

    // Switch contexts
    _asm_restore_ctx(ctx->rsp);
}

void yield_ctx_inner(void* current_rsp) {


    sp_ctx current_ctx = g_ctx.items[current_ctx_idx];
    // Save current rsp
    current_ctx->rsp = current_rsp;

    if (current_ctx_idx == 0) {
        current_ctx_idx = g_ctx.count -1; // Switch to last context
    } else {
        current_ctx_idx--; // Switch to previous context
    }

    sp_ctx target_ctx = g_ctx.items[current_ctx_idx];

    // Switch contexts
    _asm_restore_ctx(target_ctx->rsp);
}
