# Coroutine Library

A tiny coroutine runtime that lets you spin up user-space contexts with a few C calls. Stacks are allocated with `mmap`, execution is switched with a handful of assembly stubs, and the scheduler is an in-process list of contexts. Supported targets:

- Linux x86_64
- macOS aarch64 (Apple Silicon)

## Requirements
- Supported platform/arch (above)
- GCC or Clang
- No external deps; the build helper `nob` binary is included (`nob.c` if you want to rebuild it)

## Build & Run
- Build the static library and keep headers ready for linking:
  - `./nob` (release) or `./nob --debug`
  - Outputs to `build/coroutine_linux_x86_64/{lib,include}`
- Build an example: `./nob ping_pong` (produces `build/ping_pong`)
- Run an example: `./build/ping_pong`

## API at a Glance
```c
sp_ctx create_ctx(coroutine fn, void* arg); // allocate stack, schedule coroutine
void   destroy_ctx(sp_ctx ctx);             // free stack resources
bool   is_ctx_finished(sp_ctx ctx);         // has coroutine returned?

void   switch_ctx(sp_ctx ctx);              // jump to a specific coroutine
void   yield_ctx(void);                     // cooperatively yield to the scheduler

size_t get_ctx_id(void);                    // index in the scheduler list
sp_ctx get_ctx_ptr(void);                   // pointer to the current context
```

Minimal usage pattern:
```c
void worker(void* arg) {
    for (int i = 0; i < 3; i++) {
        printf("ctx %zu -> %d\n", get_ctx_id(), i);
        yield_ctx(); // hand control back
    }
}

int main(void) {
    sp_ctx a = create_ctx(worker, NULL);
    sp_ctx b = create_ctx(worker, NULL);

    while (!is_ctx_finished(a) || !is_ctx_finished(b)) {
        yield_ctx(); // drive scheduling from main
    }

    destroy_ctx(a);
    destroy_ctx(b);
}
```

## How It Works (Architecture)
**Contexts:** Each coroutine is an `s_ctx` holding `rsp`, the base of its allocated stack, and a `is_done` flag. Contexts live in a dynamic array (`g_ctx`), and `current_ctx_idx` tracks which one is executing. The main program becomes context 0 on the first `create_ctx` call.

**Stacks:** `create_ctx` uses `mmap` with `MAP_STACK | MAP_GROWSDOWN` to reserve a per-coroutine stack (`STACK_CAPACITY` = 1024 * page size). `platform_setup_stack` seeds that stack with:
- A fake return address pointing to `coroutine_finish`
- The coroutine function pointer
- Saved registers (callee-saved) and the initial argument in the right order
This makes the first `_asm_restore_ctx` place the stack exactly as if the coroutine had been called normally.

**Switching:** The assembly entry points live in `src/linux_x86_64/asm.s` (SysV) and `src/macos_aarch64/asm.s` (AAPCS64).
- `switch_ctx`: saves callee-saved registers, writes the current stack pointer into the active context, loads the target context stack, and jumps to `switch_ctx_inner` (C) which updates `current_ctx_idx` before `_asm_restore_ctx` resumes execution.
- `yield_ctx`: same register save, but selects the previous context in the list (wrapping to the last) before restoring.
- `_asm_restore_ctx`: sets the hardware stack pointer to the saved stack, restores callee-saved registers, and `ret`—the initial stack was primed so that the first return jumps into the coroutine function and that function returns into `coroutine_finish`.

**Finishing:** When a coroutine returns, control lands in `coroutine_finish`: it marks the context done, removes it from `g_ctx` without preserving order, updates `current_ctx_idx` to a valid remaining context, and restores into it. `is_ctx_finished` simply reads the flag; `destroy_ctx` unmaps and frees the context memory when you are done observing it.

**Scheduling Model:** Cooperative and minimal:
- `yield_ctx` rotates to the previous context (LIFO-ish; main is index 0).
- `switch_ctx` lets you jump directly to a known context for explicit handoffs.
- The runtime does not preempt; your coroutines must yield or switch explicitly.

## Examples
- `examples/cpt.c`: basic counter with two coroutines interleaving `yield_ctx`.
- `examples/ping_pong.c`: explicit `switch_ctx` handoff between paired coroutines.
- `examples/producer_consumer.c`: bounded buffer with cooperative backpressure.

Build any example with `./nob <name>` and run from `./build/<name>`.

## Caveats and Limits
- Single-threaded: the scheduler and context list are not thread-safe.
- Fixed stack size per coroutine (tuned via `STACK_CAPACITY` in `coroutine.c`).
- Platform-specific: only Linux x86_64 assembly/ABI is implemented.
- No signal safety or async preemption; everything is cooperative.
