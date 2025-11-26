#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <stddef.h>
#include <stdbool.h>


typedef struct s_ctx* sp_ctx;

typedef void (*coroutine) (void*);
typedef void (*finalizer) (void);

sp_ctx create_ctx(coroutine fn, void* arg);
void destroy_ctx(sp_ctx ctx);
bool is_ctx_finished(sp_ctx ctx);

void switch_ctx(sp_ctx ctx);
void yield_ctx(void);

size_t get_ctx_id(void);
sp_ctx get_ctx_ptr(void);

#endif // _COROUTINE_H
