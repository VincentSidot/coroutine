.text
.globl __asm_restore_ctx
.globl __coroutine_entry
.globl _switch_ctx
.globl _yield_ctx

/* Note: This is for macOS on ARM64 (Apple Silicon)
    - Calling convention:
        - First 8 integer/pointer arguments are passed in x0-x7
        - 9+ arguments are passed on the stack
    - Callee-saved registers: x19-x28, x29 (frame pointer), x30 (link register)
    - Stack must be 16-byte aligned at function call boundaries
    - Stack grows downwards
*/

// Restore context from saved stack pointer (x0)
__asm_restore_ctx:
    mov     sp, x0

    ldp     x29, x30, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ret

// _switch_ctx(sp_ctx ctx)
// Saves callee-saved registers and jumps to _switch_ctx_inner(current_sp, ctx)
_switch_ctx:
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    mov     x1, sp      // current stack pointer
    bl      _switch_ctx_inner

// _yield_ctx(void)
// Saves callee-saved registers and jumps to _yield_ctx_inner(current_sp)
_yield_ctx:
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    mov     x0, sp
    bl      _yield_ctx_inner

// Entry trampoline for a new coroutine
// x19: stack, x20: arg, x21: fn, x22: coroutine_finish
__coroutine_entry:
    mov     x0, x19      // stack
    mov     x1, x20      // arg
    mov     x9, x20      // fn pointer
    mov     x30, x21     // set return address to coroutine_finish
    br      x9

// Finish trampoline for a coroutine
// stack pointer is in stack frame
__coroutine_finish:
    mov     x0, sp            // stack
    bl      _coroutine_finish // jump to finish function
    // Should not return here
    brk     #0
