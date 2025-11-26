#include <stdio.h>
#include "coroutine.h"

typedef struct {
    const char* label;
    int turns;
    sp_ctx partner;
} PingPongArgs;

void ping_pong(void* data) {
    PingPongArgs* args = data;

    for (int i = 1; i <= args->turns; i++) {
        printf("[%s | ctx %zu] turn %d\n", args->label, get_ctx_id(), i);
        fflush(stdout);

        if (args->partner && !is_ctx_finished(args->partner)) {
            switch_ctx(args->partner);
        } else {
            yield_ctx();
        }
    }
}

int main(void) {
    PingPongArgs ping = {.label = "ping", .turns = 5, .partner = NULL};
    PingPongArgs pong = {.label = "pong", .turns = 5, .partner = NULL};

    sp_ctx ping_ctx = create_ctx(ping_pong, &ping);
    sp_ctx pong_ctx = create_ctx(ping_pong, &pong);

    ping.partner = pong_ctx;
    pong.partner = ping_ctx;

    switch_ctx(ping_ctx);

    while (!is_ctx_finished(ping_ctx) || !is_ctx_finished(pong_ctx)) {
        yield_ctx();
    }

    destroy_ctx(ping_ctx);
    destroy_ctx(pong_ctx);

    return 0;
}
