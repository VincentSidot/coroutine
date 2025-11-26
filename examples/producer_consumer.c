#include <stdio.h>
#include <stdbool.h>
#include "coroutine.h"

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

void producer(void* arg) {
    int limit = (int)(size_t)arg;

    for (int i = 1; i <= limit; i++) {
        while (count == BUFFER_CAP) {
            yield_ctx();
        }

        enqueue(i);
        printf("[producer | ctx %zu] produced %d (count=%zu)\n", get_ctx_id(), i, count);
        fflush(stdout);

        yield_ctx();
    }

    producer_done = true;
    yield_ctx();
}

void consumer(void* arg) {
    (void)arg;

    while (!producer_done || count > 0) {
        if (count == 0) {
            yield_ctx();
            continue;
        }

        int value = dequeue();
        printf("[consumer | ctx %zu] consumed %d (count=%zu)\n", get_ctx_id(), value, count);
        fflush(stdout);

        yield_ctx();
    }
}

int main(void) {
    sp_ctx producer_ctx = create_ctx(producer, (void*)(size_t)12);
    sp_ctx consumer_ctx = create_ctx(consumer, NULL);

    while (!is_ctx_finished(producer_ctx) || !is_ctx_finished(consumer_ctx)) {
        yield_ctx();
    }

    destroy_ctx(producer_ctx);
    destroy_ctx(consumer_ctx);

    return 0;
}
