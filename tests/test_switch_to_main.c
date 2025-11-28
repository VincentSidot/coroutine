#include <stdio.h>

#include "../src/coroutine.h"
#include "utils.h"

struct data {
  int hit_before;
  int hit_after;
};

static void jumper(sp_stack stack, void *arg) {
  struct data *d = arg;
  d->hit_before++;
  // Hop directly back to main (NULL targets main context)
  switch_ctx(stack, NULL);
  // Resume after main yields back
  d->hit_after++;
}

int main(void) {
  sp_stack stack = init_stack(0);
  struct data d = {0, 0};

  sp_ctx ctx = create_ctx(stack, jumper, &d);

  // Enter coroutine; it will switch back to main immediately
  switch_ctx(stack, ctx);

  ASSERT_TRUE(d.hit_before == 1, "jumper should run before returning to main");
  ASSERT_TRUE(d.hit_after == 0, "jumper should not have resumed yet");

  // Let coroutine finish
  yield_ctx(stack);

  ASSERT_TRUE(d.hit_after == 1, "jumper should resume after main yields");
  ASSERT_TRUE(is_ctx_finished(ctx), "ctx should finish after resuming");

  destroy_ctx(ctx);
  deinit_stack(stack);

  printf("test_switch_to_main passed\n");
  return 0;
}
