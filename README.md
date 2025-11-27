# Coroutine Library

A tiny coroutine runtime that lets you spin up user-space contexts with a few C calls. Stacks are allocated with `mmap`, execution is switched with a handful of assembly stubs, and the scheduler is an in-process list of contexts managed by an `sp_stack` handle. Supported targets:

- Linux x86_64
- macOS aarch64 (Apple Silicon)

## Requirements
- Supported platform/arch (above)
- GCC or Clang
- No external deps; the build helper `nob` binary is included (`nob.c` if you want to rebuild it)

## Build & Run
- Build the static library and headers: `./nob` (release) or `./nob --debug`
  - Outputs to `build/coroutine_<platform>/{lib,include}`
- Build an example: `./nob ping_pong` (produces `build/ping_pong`)
- Build and run in one step: `./nob --run ping_pong`
- macOS links with `-lcoroutine`; Linux links with `-l:libcoroutine.a`
- Stack size is configurable via `init_stack(<bytes>)` or the `STACK_CAPACITY` macro before including `coroutine.h`

## API at a Glance
```c
typedef void (*sp_func)(sp_stack, void*);                // coroutine signature

sp_stack init_stack(size_t stack_capacity);             // create scheduler handle (0 -> default size)
void     deinit_stack(sp_stack stack);                  // tear down (all coroutines must be destroyed)

sp_ctx   create_ctx(sp_stack stack, sp_func fn, void*); // allocate stack, schedule coroutine
void     destroy_ctx(sp_ctx ctx);                       // free stack resources
bool     is_ctx_finished(sp_ctx ctx);                   // has coroutine returned?

void     switch_ctx(sp_stack stack, sp_ctx ctx);        // jump to a specific coroutine (NULL -> main)
void     yield_ctx(sp_stack stack);                     // cooperatively yield to the scheduler

sp_ctx   get_ctx(sp_stack stack);                       // pointer to the current context (NULL in main)
```

Minimal usage pattern:
```c
void worker(sp_stack stack, void* arg) {
    int* limit = arg;

    for (int i = 0; i < *limit; i++) {
        printf("ctx %p -> %d\n", (void*)get_ctx(stack), i);
        yield_ctx(stack); // hand control back
    }
}

int main(void) {
    sp_stack stack = init_stack(0); // default capacity

    int limit = 3;
    sp_ctx a = create_ctx(stack, worker, &limit);
    sp_ctx b = create_ctx(stack, worker, &limit);

    while (!is_ctx_finished(a) || !is_ctx_finished(b)) {
        yield_ctx(stack); // drive scheduling from main
    }

    destroy_ctx(a);
    destroy_ctx(b);
    deinit_stack(stack);
}
```

## How It Works (Architecture)
**Contexts:** Each coroutine is an `s_ctx` holding `rsp`, the base of its allocated stack, a `is_done` flag, and the stack size used for allocation. Contexts live inside an `s_stack` handle (`active_ctxs`), and `current_index` tracks which one is executing. The main program becomes context 0 when you call `init_stack`.

**Stacks:** `create_ctx` uses `mmap` with `MAP_STACK | MAP_GROWSDOWN` to reserve a per-coroutine stack (`STACK_CAPACITY` defaults to `1024 * getpagesize()`, overridable via `init_stack`). `platform_setup_stack` seeds that stack with:
- Return plumbing that routes the coroutine back into `coroutine_finish`
- The coroutine function pointer and its `(sp_stack, void*)` arguments
- Saved registers (callee-saved) and the initial argument in the right order
This makes the first `_asm_restore_ctx` place the stack exactly as if the coroutine had been called normally.

**Switching:** The assembly entry points live in `src/linux_x86_64/asm.s` (SysV) and `src/macos_aarch64/asm.s` (AAPCS64).
- `switch_ctx`: saves callee-saved registers, writes the current stack pointer into the active context, loads the target context stack, and jumps to `switch_ctx_inner` (C) which updates `current_index` before `_asm_restore_ctx` resumes execution.
- `yield_ctx`: same register save, but selects the previous context in the list (wrapping to the last active coroutine) before restoring.
- `_asm_restore_ctx`: sets the hardware stack pointer to the saved stack, restores callee-saved registers, and `ret`—the initial stack was primed so that the first return jumps into the coroutine function and that function returns into `coroutine_finish`.

**Finishing:** When a coroutine returns, control lands in `coroutine_finish`: it marks the context done, removes it from the active set, updates `current_index` to a valid remaining context, and restores into it. `is_ctx_finished` simply reads the flag; `destroy_ctx` unmaps and frees the context memory when you are done observing it.

**Scheduling Model:** Cooperative and minimal:
- `yield_ctx` rotates to the previous context within the `sp_stack` (LIFO-ish; main is index 0).
- `switch_ctx` lets you jump directly to a known context for explicit handoffs within the same `sp_stack`.
- The runtime does not preempt; your coroutines must yield or switch explicitly.

## Examples
- `examples/hello.c`: smallest possible coroutine handshake with `yield_ctx`.
- `examples/cpt.c`: basic counter with two coroutines interleaving `yield_ctx`.
- `examples/ping_pong.c`: explicit `switch_ctx` handoff between paired coroutines on one stack.
- `examples/producer_consumer.c`: bounded buffer with cooperative backpressure.

Build any example with `./nob <name>` and run from `./build/<name>`.
