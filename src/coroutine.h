#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <stddef.h>
#include <stdbool.h>


typedef struct s_ctx* sp_ctx;

typedef void (*coroutine) (void*);
typedef void (*finalizer) (void);

extern sp_ctx create_ctx(coroutine fn, void* arg);
extern void destroy_ctx(sp_ctx ctx);
extern bool is_ctx_finished(sp_ctx ctx);

extern void switch_ctx(sp_ctx ctx);
extern void yield_ctx(void);

extern size_t get_ctx_id(void);
extern sp_ctx get_ctx_ptr(void);

#endif // _COROUTINE_H
