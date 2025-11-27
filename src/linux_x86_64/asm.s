.global _asm_restore_ctx
.global _coroutine_finish
.global yield_ctx
.global switch_ctx

/* Note: This implementation is for linux x86_64 architecture

## Architecture specifics:
- Stack grows downwards
- Stack must be 16-byte aligned at function call
- Non volatile registers are: rbx, rbp, r12, r13, r14, r15
- Calling convention:
    - First 6 integer/pointer arguments are passed in rdi, rsi, rdx, rcx, r8, r9
    - 7+ arguments are passed on the stack

*/


/*
    * Restore Context stack.
    * Input: rdi - Context rsp address
*/
_asm_restore_ctx:
    movq %rdi, %rsp           /* Set stack pointer to context rsp */

    popq %r15                 /* Restore r15 */
    popq %r14                 /* Restore r14 */
    popq %r13                 /* Restore r13 */
    popq %r12                 /* Restore r12 */
    popq %rbp                 /* Restore rbp */
    popq %rbx                 /* Restore rbx */
    popq %rsi                 /* Restore rsi */
    popq %rdi                 /* Restore rdi */

    /* We can return here because by design the return address is on top of the stack */
    ret                       /* Return to the restored context */

/*
    * Coroutine fishish trampoline.
    * Input: stack - Pointer to the stack pointer
*/
_coroutine_finish:
    popq %rdi                 /* Get the stack pointer address */
    subq $8, %rsp             /* Align stack to 16 bytes */
    jmp coroutine_finish      /* Jump to the finish handler */

/*
    * Switch Context stack.
    * Input: rdi - Pointer to the stack context
    * Input: rsi - Pointer to the target context rsp address
*/
switch_ctx:
    pushq %rdi              /* Save rdi */
    pushq %rsi              /* Save rsi */
    pushq %rbx              /* Save rbx */
    pushq %rbp              /* Save rbp */
    pushq %r12              /* Save r12 */
    pushq %r13              /* Save r13 */
    pushq %r14              /* Save r14 */
    pushq %r15              /* Save r15 */

    movq %rsp, %rdx         /* Load new rsp from the context */

    jmp switch_ctx_inner    /* Jump to inner switch function */

/*
    * Yield Context stack.
    * Input: rdi - Pointer to the stack context
*/
yield_ctx:
    pushq %rdi              /* Save rdi */
    pushq %rsi              /* Save rsi */
    pushq %rbx              /* Save rbx */
    pushq %rbp              /* Save rbp */
    pushq %r12              /* Save r12 */
    pushq %r13              /* Save r13 */
    pushq %r14              /* Save r14 */
    pushq %r15              /* Save r15 */

    movq %rsp, %rsi         /* Load new rsp from the context */

    jmp yield_ctx_inner     /* Jump to inner yield function */
