#include "../coroutine.h"



void* platform_setup_stack(void** rsp, coroutine fn, finalizer coroutine_finish, void* arg) {
    *(--rsp) = coroutine_finish;    // ret addr
    *(--rsp) = fn;                  // fn

    *(--rsp) = arg;                 // push rdi (arg)
    *(--rsp) = 0;                   // push rbx
    *(--rsp) = 0;                   // push rbp
    *(--rsp) = 0;                   // push r12
    *(--rsp) = 0;                   // push r13
    *(--rsp) = 0;                   // push r14
    *(--rsp) = 0;                   // push r15
    --rsp;                          // alignment padding

    return rsp;
}
