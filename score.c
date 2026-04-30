#include "engine.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define FEED_COUNT 200000

static t_order feed[FEED_COUNT];
static t_orderid ids[FEED_COUNT];
static volatile uint64_t executionChecksum;
static volatile unsigned long executionCount;

void execution(t_execution exec) {
  executionChecksum ^= ((uint64_t)exec.price << 32) ^ exec.size ^
                       ((uint64_t)(unsigned char)exec.trader[0] << 8) ^
                       (uint64_t)(unsigned char)exec.side;
  executionCount++;
}

static uint32_t xorshift32(uint32_t *state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

static void set_field(char dst[FIELD_LEN], char a, unsigned int n) {
  dst[0] = a;
  dst[1] = (char)('0' + ((n / 100) % 10));
  dst[2] = (char)('0' + ((n / 10) % 10));
  dst[3] = (char)('0' + (n % 10));
  dst[4] = '\0';
}

static void generate_feed(void) {
  uint32_t state = 0xC0FFEEu;
  static const char symbols[][FIELD_LEN] = {"AAPL", "MSFT", "NVDA", "TSLA"};

  for (int i = 0; i < FEED_COUNT; i++) {
    uint32_t r = xorshift32(&state);
    t_order *order = &feed[i];
    memcpy(order->symbol, symbols[r & 3u], FIELD_LEN);
    order->side = (r >> 7) & 1u;
    set_field(order->trader, order->side ? 'S' : 'B', (unsigned int)i % 1000u);

    /*
      Keep prices clustered so most orders inspect only a few price points,
      matching the shape of the QuantCup benchmark.
    */
    order->price = 30000u + ((r >> 8) % 401u) - 200u;
    if (order->price < MIN_PRICE) {
      order->price = MIN_PRICE;
    } else if (order->price > MAX_PRICE) {
      order->price = MAX_PRICE;
    }
    order->size = 1u + ((r >> 20) % 50u);
  }
}

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main(void) {
  double start;
  double elapsed;

  generate_feed();
  executionChecksum = 0;
  executionCount = 0;

  init();
  start = now_ms();
  for (int i = 0; i < FEED_COUNT; i++) {
    ids[i] = limit(feed[i]);
    if (i > 64 && (i % 17) == 0) {
      cancel(ids[i - 31]);
    }
  }
  elapsed = now_ms() - start;
  destroy();

  printf("orders=%d executions=%lu checksum=%llu time_ms=%.2f\n", FEED_COUNT,
         executionCount, (unsigned long long)executionChecksum, elapsed);
  return 0;
}
