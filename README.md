# Coroutine Playground (Linux x86_64)

This repository implements a tiny user-space coroutine/scheduler for Linux x86_64 with hand-written assembly. It is intentionally minimal so you can study how stacks and register frames are built and swapped.

## Quick Start
- Build helper: `cc -o nob nob.c`
- Build demo: `./nob` → emits `build/app`
- Run demo: `./build/app` (prints coroutine interleaving with context ids and addresses)

## How It Works

### Data Structures
- Context type: `struct s_ctx` in `src/coroutine.c` holds:
  - `rsp`: saved stack pointer for the coroutine
  - `stack_base`: mmap’ed stack memory base
  - `is_done`: completion flag
- Global registry: `g_ctx` (dynamic array from `array.h`) stores all active contexts. `current_ctx_idx` tracks which entry is running (index `0` is the implicit main context).

### Stack Creation
1. `create_ctx(fn, arg)` (in `src/coroutine.c`) lazily creates the main context and then allocates a coroutine stack with `mmap` sized at `STACK_CAPACITY` (`1024 * getpagesize()`).
2. The stack top pointer (`stack_base + STACK_CAPACITY`) is passed to `platform_setup_stack` (`src/linux_x86_64/platform.c`), which lays out an initial frame:
   - Return address → `coroutine_finish` (runs when `fn` returns).
   - Next return address → `fn` (so `ret` jumps into the coroutine body).
   - Callee-saved registers (`r15`…`rbx`, `rbp`) and `rdi` slot with the argument.
   - 16-byte alignment padding for the System V ABI.
3. The prepared stack pointer is stored in the context and pushed into `g_ctx`.

### Execution Flow
1. The demo (`src/main.c`) creates two contexts and then repeatedly calls `yield_ctx()`.
2. `yield_ctx` is defined in assembly (`src/linux_x86_64/asm.s`). It saves callee-saved registers onto the current stack, aligns the stack, moves the new `rsp` into `rdi`, and jumps to `yield_ctx_inner` (C).
3. `yield_ctx_inner` saves the current `rsp` into the current context, advances `current_ctx_idx` to the previous entry (wrapping from `0` to last), loads the target context, and calls `_asm_restore_ctx`.
4. `_asm_restore_ctx` switches stacks by moving the saved `rsp` into `%rsp`, pops registers in reverse, restores `rdi`, and executes `ret`. Control lands either back inside another coroutine or, if it returns from the coroutine body, into `coroutine_finish`.
5. `coroutine_finish` marks the context as done, removes it from `g_ctx`, and jumps back to the prior context with `_asm_restore_ctx`.

### Assembly Primitives
- `yield_ctx`: save registers, align stack, jump into C scheduler to pick the next context.
- `switch_ctx`: similar to `yield_ctx` but switches to an explicit target (`switch_ctx_inner`).
- `_asm_restore_ctx`: load a saved stack frame and return into whatever return address was staged during stack setup.

### Scheduler Notes
- Scheduling is cooperative and round-robin over the `g_ctx` array (reverse order: main → last → …). Contexts voluntarily yield; there is no preemption.
- Stacks are per-coroutine and freed via `destroy_ctx` (manual; not invoked in the demo).
- `is_ctx_finished` reports completion; the demo loops until all non-main contexts are done.

## Files of Interest
- `src/coroutine.c` / `src/coroutine.h` – context management, scheduler core.
- `src/linux_x86_64/asm.s` – register save/restore and context switch glue.
- `src/linux_x86_64/platform.c` – stack frame bootstrap for new coroutines.
- `nob.c` / `nob.h` – tiny build helper that compiles the demo.

## Extending / Porting
- Add a new architecture directory under `src/<arch>/` with equivalents of `asm.s` and `platform.c`.
- Verify stack alignment rules and callee-saved registers for the target ABI.
- Consider adding test harnesses (e.g., stack overflow checks, nested yields, destruction paths).
