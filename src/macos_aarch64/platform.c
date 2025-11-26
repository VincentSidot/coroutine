#include "../coroutine.h"

extern void _coroutine_entry(void);

void* platform_setup_stack(void* stack_top, coroutine fn, finalizer coroutine_finish, void* arg) {
    // Stack grows down. We mirror the layout produced by yield/switch:
    // [x29][x30][x27][x28][x25][x26][x23][x24][x21][x22][x19][x20]
    // Registers are restored in that order, so we seed initial values.

    unsigned long* sp = (unsigned long*)stack_top;

    // Helper macro to push two registers (second placed at higher address)
#define PUSH_PAIR(low, high) \
    do { *(--sp) = (unsigned long)(high); *(--sp) = (unsigned long)(low); } while (0)

    PUSH_PAIR(arg, fn);                 // x19, x20
    PUSH_PAIR(coroutine_finish, 0);     // x21, x22
    PUSH_PAIR(0, 0);                    // x23, x24
    PUSH_PAIR(0, 0);                    // x25, x26
    PUSH_PAIR(0, 0);                    // x27, x28
    PUSH_PAIR(0, _coroutine_entry);      // x29 (fp), x30 (lr -> trampoline)

#undef PUSH_PAIR

    return sp;
}
