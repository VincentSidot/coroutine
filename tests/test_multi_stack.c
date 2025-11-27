#include <stdio.h>
#include "coroutine.h"

#define ASSERT_TRUE(cond, msg)                                                \
  do {                                                                        \
    if (!(cond)) {                                                            \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));        \
      return 1;                                                               \
    }                                                                         \
  } while (0)

struct counter {
  int value;
};

static void tick(sp_stack stack, void *arg) {
  struct counter *c = arg;
  c->value++;
  yield_ctx(stack);
  c->value++;
}

int main(void) {
  sp_stack stack1 = init_stack(0);
  sp_stack stack2 = init_stack(0);

  struct counter c1 = {0};
  struct counter c2 = {0};

  sp_ctx ctx1 = create_ctx(stack1, tick, &c1);
  sp_ctx ctx2 = create_ctx(stack2, tick, &c2);

  // Drive first stack to completion
  while (!is_ctx_finished(ctx1)) {
    yield_ctx(stack1);
  }

  // Drive second stack to completion
  while (!is_ctx_finished(ctx2)) {
    yield_ctx(stack2);
  }

  ASSERT_TRUE(c1.value == 2, "stack1 counter should tick twice");
  ASSERT_TRUE(c2.value == 2, "stack2 counter should tick twice");

  destroy_ctx(ctx1);
  destroy_ctx(ctx2);
  deinit_stack(stack1);
  deinit_stack(stack2);

  printf("test_multi_stack passed\n");
  return 0;
}
