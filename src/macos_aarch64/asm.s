.text
.globl _asm_restore_ctx
.globl _coroutine_entry
.globl switch_ctx
.globl yield_ctx

// Restore context from saved stack pointer (x0)
_asm_restore_ctx:
    mov     sp, x0

    ldp     x29, x30, [sp], #16
    ldp     x27, x28, [sp], #16
    ldp     x25, x26, [sp], #16
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ret

// switch_ctx(sp_ctx ctx)
// Saves callee-saved registers and jumps to switch_ctx_inner(current_sp, ctx)
switch_ctx:
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    mov     x1, x0      // ctx
    mov     x0, sp      // current stack pointer
    bl      switch_ctx_inner

// yield_ctx(void)
// Saves callee-saved registers and jumps to yield_ctx_inner(current_sp)
yield_ctx:
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!
    stp     x25, x26, [sp, #-16]!
    stp     x27, x28, [sp, #-16]!
    stp     x29, x30, [sp, #-16]!

    mov     x0, sp
    bl      yield_ctx_inner

// Entry trampoline for a new coroutine
// x19: arg, x20: fn, x21: coroutine_finish
_coroutine_entry:
    mov     x0, x19      // arg
    mov     x9, x20      // fn pointer
    mov     x30, x21     // set return address to coroutine_finish
    br      x9
