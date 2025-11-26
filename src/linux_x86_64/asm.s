.global _asm_restore_ctx
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

    addq $8, %rsp             /* Skip the dummy value */
    popq %r15                 /* Restore r15 */
    popq %r14                 /* Restore r14 */
    popq %r13                 /* Restore r13 */
    popq %r12                 /* Restore r12 */
    popq %rbx                 /* Restore rbx */
    popq %rbp                 /* Restore rbp */
    popq %rdi                 /* Restore rdi */

    /* We can return here because by design the return address is on top of the stack */
    ret                       /* Return to the restored context */

/*
    * Switch Context stack.
    * Input: rdi - Pointer to the target context rsp address
*/
switch_ctx:
    pushq %rdi              /* Save rdi */
    pushq %rbp              /* Save rbp */
    pushq %rbx              /* Save rbx */
    pushq %r12              /* Save r12 */
    pushq %r13              /* Save r13 */
    pushq %r14              /* Save r14 */
    pushq %r15              /* Save r15 */
    subq $8, %rsp           /* Align stack to 16 bytes */

    movq %rsp, %rsi         /* Load new rsp from the context */

    jmp switch_ctx_inner    /* Jump to inner switch function */

/*
    * Yield Context stack.
*/
yield_ctx:
    pushq %rdi              /* Save rdi */
    pushq %rbp              /* Save rbp */
    pushq %rbx              /* Save rbx */
    pushq %r12              /* Save r12 */
    pushq %r13              /* Save r13 */
    pushq %r14              /* Save r14 */
    pushq %r15              /* Save r15 */
    subq $8, %rsp           /* Align stack to 16 bytes */

    movq %rsp, %rdi         /* Load new rsp from the context */

    jmp yield_ctx_inner     /* Jump to inner yield function */
