#include <stdbool.h>
#include <stdio.h>

#include "../src/coroutine.h"

#define BUFFER_CAP 4

static int buffer[BUFFER_CAP];
static size_t head = 0;
static size_t tail = 0;
static size_t count = 0;
static bool producer_done = false;

static void enqueue(int value) {
  buffer[tail] = value;
  tail = (tail + 1) % BUFFER_CAP;
  count++;
}

static int dequeue(void) {
  int value = buffer[head];
  head = (head + 1) % BUFFER_CAP;
  count--;
  return value;
}

void producer(sp_stack stack, void *arg) {
  int limit = (int)(size_t)arg;

  for (int i = 1; i <= limit; i++) {
    while (count == BUFFER_CAP) {
      yield_ctx(stack);
    }

    enqueue(i);
    printf("[producer | ctx %p] produced %d (count=%zu)\n",
           (void *)get_ctx(stack), i, count);
    fflush(stdout);

    yield_ctx(stack);
  }

  producer_done = true;
  yield_ctx(stack);
}

void consumer(sp_stack stack, void *arg) {
  (void)arg;

  while (!producer_done || count > 0) {
    if (count == 0) {
      yield_ctx(stack);
      continue;
    }

    int value = dequeue();
    printf("[consumer | ctx %p] consumed %d (count=%zu)\n",
           (void *)get_ctx(stack), value, count);
    fflush(stdout);

    yield_ctx(stack);
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  sp_stack stack = init_stack(16384);

  sp_ctx producer_ctx = create_ctx(stack, producer, (void *)(size_t)12);
  sp_ctx consumer_ctx = create_ctx(stack, consumer, NULL);

  while (!is_ctx_finished(producer_ctx) || !is_ctx_finished(consumer_ctx)) {
    yield_ctx(stack);
  }

  destroy_ctx(producer_ctx);
  destroy_ctx(consumer_ctx);

  deinit_stack(stack);

  return 0;
}
