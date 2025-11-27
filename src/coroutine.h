#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <stdbool.h>
#include <stddef.h>

// User can define STACK_CAPACITY before including this header
#ifndef STACK_CAPACITY
#define STACK_CAPACITY (1024 * getpagesize())
#endif // STACK_CAPACITY

// Opaque coroutine context type
typedef struct s_ctx *sp_ctx;

// Opaque stack type
typedef struct s_stack *sp_stack;

/**
 * Coroutine function type and finalizer function type
 * coroutine: function that takes a void* argument and returns void
 */
typedef void (*sp_func)(sp_stack, void *);

/*
 * Coroutine management functions
 */

/**
 * @brief Initialize a new stack for coroutines
 * @param stack_capacity The capacity of the stack in bytes (if 0, use default
 * STACK_CAPACITY)
 * @return Stack object
 */
extern sp_stack init_stack(size_t stack_capacity);

extern void deinit_stack(sp_stack stack);

/**
 * @brief Create a new coroutine context
 * @param fn The coroutine function to execute
 * @param arg The argument to pass to the coroutine function
 * @return Coroutine context object
 */
extern sp_ctx create_ctx(sp_stack stack, sp_func fn, void *arg);

/**
 * @brief Unregister a coroutine context from the stack
 * @param stack The stack containing the coroutine context
 * @param ctx The coroutine context to unregister
 */
extern void unregister_ctx(sp_stack stack, sp_ctx ctx);

/**
 * @brief Destroy a coroutine context
 * @param ctx The coroutine context to destroy
 * @warning Cannot destroy the main context or a non-finished context
 */
extern void destroy_ctx(sp_ctx ctx);

/**
 * @brief Check if a coroutine context has finished execution
 * @param ctx The coroutine context to check
 * @return true if the coroutine has finished, false otherwise
 */
extern bool is_ctx_finished(sp_ctx ctx);

/**
 * @brief Switch to the given coroutine context
 * @param ctx The coroutine context to switch to (NULL for main context)
 */
extern void switch_ctx(sp_stack stack, sp_ctx ctx);

/**
 * @brief Yield execution from the current coroutine context back to the caller
 */
extern void yield_ctx(sp_stack stack);

/**
 * @brief Get a pointer to the current coroutine context
 * @return Pointer to the current coroutine context (NULL for main context)
 */
extern sp_ctx get_ctx(sp_stack stack);

#endif // _COROUTINE_H
