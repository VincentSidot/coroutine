#include <stdio.h>
#include <stdlib.h>

#include "coroutine.h"

#define ASSERT_TRUE(cond, msg)                                                \
  do {                                                                        \
    if (!(cond)) {                                                            \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (msg));        \
      return 1;                                                               \
    }                                                                         \
  } while (0)

#define ASSERT_EQ_INT(actual, expected, msg)                                  \
  ASSERT_TRUE((actual) == (expected), (msg))

struct hit_data {
  int hits;
};

static void two_step(sp_stack stack, void *arg) {
  struct hit_data *data = arg;
  data->hits++;
  yield_ctx(stack);
  data->hits++;
}

static int test_yield_round_robin(void) {
  sp_stack stack = init_stack(0);
  struct hit_data a = {0};
  struct hit_data b = {0};

  sp_ctx ctx_a = create_ctx(stack, two_step, &a);
  sp_ctx ctx_b = create_ctx(stack, two_step, &b);

  while (!is_ctx_finished(ctx_a) || !is_ctx_finished(ctx_b)) {
    yield_ctx(stack);
  }

  destroy_ctx(ctx_a);
  destroy_ctx(ctx_b);
  deinit_stack(stack);

  ASSERT_EQ_INT(a.hits, 2, "ctx A should hit twice");
  ASSERT_EQ_INT(b.hits, 2, "ctx B should hit twice");
  return 0;
}

struct call_counter {
  int called;
};

static void one_shot(sp_stack stack, void *arg) {
  (void)stack;
  struct call_counter *counter = arg;
  counter->called++;
}

static int test_switch_direct(void) {
  sp_stack stack = init_stack(0);
  struct call_counter counter = {0};

  sp_ctx ctx = create_ctx(stack, one_shot, &counter);

  switch_ctx(stack, ctx);

  ASSERT_EQ_INT(counter.called, 1, "one_shot should run exactly once");
  ASSERT_TRUE(is_ctx_finished(ctx), "ctx should be finished after return");

  destroy_ctx(ctx);
  deinit_stack(stack);
  return 0;
}

int main(void) {
  int failures = 0;

  failures += test_yield_round_robin();
  failures += test_switch_direct();

  if (failures == 0) {
    printf("All tests passed\n");
    return 0;
  }

  fprintf(stderr, "Tests failed: %d\n", failures);
  return 1;
}
