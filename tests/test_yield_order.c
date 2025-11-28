#include <stdio.h>
#include <stdlib.h>

#include "../src/coroutine.h"
#include "utils.h"

struct rec {
  int id;
  int steps;
  int *log;
  int *len;
};

static void logger(sp_stack stack, void *arg) {
  struct rec *r = arg;
  for (int i = 0; i < r->steps; i++) {
    r->log[(*r->len)++] = r->id;
    yield_ctx(stack);
  }
}

int main(void) {
  sp_stack stack = init_stack(0);

  int log[32] = {0};
  int len = 0;

  struct rec a = {.id = 1, .steps = 2, .log = log, .len = &len};
  struct rec b = {.id = 2, .steps = 2, .log = log, .len = &len};

  sp_ctx ctx_a = create_ctx(stack, logger, &a);
  sp_ctx ctx_b = create_ctx(stack, logger, &b);

  // Drive until both coroutines finish
  while (!is_ctx_finished(ctx_a) || !is_ctx_finished(ctx_b)) {
    yield_ctx(stack);
    // mark main activity as 0 for visibility
    log[len++] = 0;
  }

  destroy_ctx(ctx_a);
  destroy_ctx(ctx_b);
  deinit_stack(stack);

  // Both coroutines should have logged exactly their steps
  int count_a = 0, count_b = 0, count_main = 0;
  for (int i = 0; i < len; i++) {
    if (log[i] == 1)
      count_a++;
    else if (log[i] == 2)
      count_b++;
    else if (log[i] == 0)
      count_main++;
  }

  ASSERT_TRUE(count_a == a.steps, "Coroutine A log count mismatch");
  ASSERT_TRUE(count_b == b.steps, "Coroutine B log count mismatch");
  ASSERT_TRUE(count_main >= 1, "Main should have logged activity");

  // The first non-main entry should be from the last created coroutine (LIFO)
  int first_non_main = -1;
  for (int i = 0; i < len; i++) {
    if (log[i] != 0) {
      first_non_main = log[i];
      break;
    }
  }
  ASSERT_TRUE(first_non_main == 2,
              "Yield rotation should start with newest ctx");

  printf("test_yield_order passed\n");
  return 0;
}
