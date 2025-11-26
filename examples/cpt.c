#include <stdio.h>
#include "coroutine.h"

void cpt(int cpt) {
    for(int i = 0; i < cpt; i++) {
        printf("[%p |%3zu] i = %d\n", get_ctx_ptr(), get_ctx_id(), i);
        fflush(stdout);
        // switch_ctx(ctx[other_idx]);
        yield_ctx();
    }

    return;
}

int main() {
    sp_ctx ctx1 = create_ctx((void*) cpt, (void*)(size_t)10);
    sp_ctx ctx2 = create_ctx((void*) cpt, (void*)(size_t)25);


    while (!is_ctx_finished(ctx1) || !is_ctx_finished(ctx2)) {
        yield_ctx();
    }
}
