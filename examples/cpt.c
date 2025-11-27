#include <stdio.h>
#include "coroutine.h"

void cpt(sp_stack stack, int cpt) {

    printf("Running coroutine cpt for %d\n", cpt);

    for(int i = 0; i < cpt; i++) {
        printf("[%p] i = %d\n", get_ctx(stack),i);
        fflush(stdout);

        yield_ctx(stack);
    }

    return;
}

int main() {
    sp_stack stack = init_stack(0); // default stack size


    sp_ctx ctx1 = create_ctx(stack, (void*) cpt, (void*)(size_t)10);
    sp_ctx ctx2 = create_ctx(stack, (void*) cpt, (void*)(size_t)25);


    while (!is_ctx_finished(ctx1) || !is_ctx_finished(ctx2)) {
        yield_ctx(stack);
    }

    destroy_ctx(ctx1);
    destroy_ctx(ctx2);

    deinit_stack(stack);
}
