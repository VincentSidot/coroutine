#include <assert.h>
#include <stdio.h>

#include "../src/coroutine.h"

void greet(sp_stack stack, char *name) {
  printf("Oh hi %s!\n", name);
  fflush(stdout);

  yield_ctx(stack);

  printf("Welcome %s, to the coroutine world!\n", name);
  fflush(stdout);

  return;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  sp_stack stack = init_stack(0);

  sp_ctx ctx = create_ctx(stack, (sp_func)greet, "Marc");

  yield_ctx(stack); // Should print the greeting

  printf("Should be back in main now.\n");

  yield_ctx(stack); // Should print the welcome message

  printf("Back in main again.\n");

  assert(is_ctx_finished(ctx) && "Coroutine should be done");

  destroy_ctx(ctx);
  deinit_stack(stack);

  return 0;
}
