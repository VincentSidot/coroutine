#include <stdio.h>

#include "../src/coroutine.h"

typedef struct {
  const char *label;
  int turns;
  sp_ctx partner;
} PingPongArgs;

void ping_pong(sp_stack stack, void *args) {
  PingPongArgs *ping = (PingPongArgs *)args;

  for (int i = 1; i <= ping->turns; i++) {
    printf("[%s | ctx %p] turn %d\n", ping->label, (void *)get_ctx(stack), i);
    fflush(stdout);

    if (ping->partner && !is_ctx_finished(ping->partner)) {
      switch_ctx(stack, ping->partner);
    } else {
      yield_ctx(stack);
    }
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  PingPongArgs ping = {.label = "ping", .turns = 5, .partner = NULL};
  PingPongArgs pong = {.label = "pong", .turns = 5, .partner = NULL};

  sp_stack stack = init_stack(16384);

  sp_ctx ping_ctx = create_ctx(stack, ping_pong, &ping);
  sp_ctx pong_ctx = create_ctx(stack, ping_pong, &pong);

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
