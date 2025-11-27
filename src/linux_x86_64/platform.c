#include "../coroutine.h"
#include <stdint.h>

extern void _coroutine_finish(void);


void* platform_setup_stack(void* stack_top, sp_func fn, sp_stack stack, void* arg) {

    uint64_t *rsp = (uint64_t*)stack_top;

    #define PUSH(val) *(--rsp) = (uint64_t)(val)

    // Push initial stack frame
    --rsp;                     // align stack
    PUSH(stack);               // stack arg for coroutine_finish
    PUSH(_coroutine_finish);   // ret addr
    PUSH(fn);                  // fn

    PUSH(stack);               // push rdi (stack)
    PUSH(arg);                 // push rsi (arg)
    PUSH(0);                   // push rbx
    PUSH(0);                   // push rbp
    PUSH(0);                   // push r12
    PUSH(0);                   // push r13
    PUSH(0);                   // push r14
    PUSH(0);                   // push r15


    return rsp;
}
