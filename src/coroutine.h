#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <stddef.h>
#include <stdbool.h>

// User can define STACK_CAPACITY before including this header
#ifndef STACK_CAPACITY
    #define STACK_CAPACITY (1024*getpagesize())
#endif // STACK_CAPACITY

// Opaque coroutine context type
typedef struct s_ctx* sp_ctx;

/**
 * Coroutine function type and finalizer function type
 * coroutine: function that takes a void* argument and returns void
 */
typedef void (*coroutine) (void*);

// Internal finalizer function type
typedef void (*finalizer) (void);

/*
    * Coroutine management functions
*/

/**
 * @brief Create a new coroutine context
 * @param fn The coroutine function to execute
 * @param arg The argument to pass to the coroutine function
 * @return Coroutine context object
 */
extern sp_ctx create_ctx(coroutine fn, void* arg);

/**
 * @brief Destroy a coroutine context
 * @param ctx The coroutine context to destroy
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
extern void switch_ctx(sp_ctx ctx);

/**
 * @brief Yield execution from the current coroutine context back to the caller
 */
extern void yield_ctx(void);

/**
 * @brief Get the ID of the current coroutine context
 * @return The ID of the current coroutine context (0 for main context)
 */
extern size_t get_ctx_id(void);

/**
 * @brief Get a pointer to the current coroutine context
 * @return Pointer to the current coroutine context (NULL for main context)
 */
extern sp_ctx get_ctx_ptr(void);

#endif // _COROUTINE_H
