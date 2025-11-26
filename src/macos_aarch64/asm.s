.text
.globl __asm_restore_ctx
.globl __coroutine_entry
.globl _switch_ctx
.globl _yield_ctx

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

    mov     x1, x0      // ctx
    mov     x0, sp      // current stack pointer
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
// x19: arg, x20: fn, x21: coroutine_finish
__coroutine_entry:
    mov     x0, x19      // arg
    mov     x9, x20      // fn pointer
    mov     x30, x21     // set return address to coroutine_finish
    br      x9
