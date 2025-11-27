#include <stdio.h>
#include "coroutine.h"

typedef struct {
    const char* label;
    int turns;
    sp_ctx partner;
} PingPongArgs;

void ping_pong(sp_stack stack, PingPongArgs* args) {

    for (int i = 1; i <= args->turns; i++) {
        printf("[%s | ctx %p] turn %d\n", args->label, get_ctx(stack), i);
        fflush(stdout);

        if (args->partner && !is_ctx_finished(args->partner)) {
            switch_ctx(stack, args->partner);
        } else {
            yield_ctx(stack);
        }
    }
}

int main(void) {
    PingPongArgs ping = {.label = "ping", .turns = 5, .partner = NULL};
    PingPongArgs pong = {.label = "pong", .turns = 5, .partner = NULL};

    sp_stack stack = init_stack(16384);

    sp_ctx ping_ctx = create_ctx(stack, (void*)ping_pong, &ping);
    sp_ctx pong_ctx = create_ctx(stack, (void*)ping_pong, &pong);

    ping.partner = pong_ctx;
    pong.partner = ping_ctx;

    switch_ctx(stack, ping_ctx);

    while (!is_ctx_finished(ping_ctx) || !is_ctx_finished(pong_ctx)) {
        yield_ctx(stack);
    }

    destroy_ctx(ping_ctx);
    destroy_ctx(pong_ctx);

    deinit_stack(stack);

    return 0;
}
