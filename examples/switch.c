#include <stdio.h>

#include "../src/coroutine.h"

void greet(sp_stack stack, char *name) {
  printf("Hello, %s!\n", name);
  // Yield back to main context
  yield_ctx(stack);
  printf("Goodbye, %s!\n", name);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  sp_stack stack = init_stack(0);

  sp_ctx ctx = create_ctx(stack, (sp_func)greet, "Alice");

  yield_ctx(stack);

  unregister_ctx(stack, ctx);
  destroy_ctx(ctx);

  yield_ctx(stack);

  printf("Back to main context.\n");

  deinit_stack(stack);

  return 0;
}
