#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EXECS 256

static t_execution execs[MAX_EXECS];
static size_t execCount;

void execution(t_execution exec) {
  if (execCount >= MAX_EXECS) {
    fprintf(stderr, "too many executions\n");
    exit(1);
  }
  execs[execCount++] = exec;
}

static t_order order4(const char *symbol, const char *trader, t_side side,
                      t_price price, t_size size) {
  t_order order;
  memset(&order, 0, sizeof(order));
  memcpy(order.symbol, symbol, FIELD_CHARS);
  memcpy(order.trader, trader, FIELD_CHARS);
  order.symbol[FIELD_CHARS] = '\0';
  order.trader[FIELD_CHARS] = '\0';
  order.side = side;
  order.price = price;
  order.size = size;
  return order;
}

static void reset_book(void) {
  init();
  execCount = 0;
  memset(execs, 0, sizeof(execs));
}

static void require_int(int condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
  }
}

static void require_exec(size_t idx, const char *symbol, const char *trader,
                         t_side side, t_price price, t_size size) {
  char message[160];
  snprintf(message, sizeof(message), "execution %zu exists", idx);
  require_int(idx < execCount, message);
  snprintf(message, sizeof(message), "execution %zu symbol", idx);
  require_int(strcmp(execs[idx].symbol, symbol) == 0, message);
  snprintf(message, sizeof(message), "execution %zu trader", idx);
  require_int(strcmp(execs[idx].trader, trader) == 0, message);
  snprintf(message, sizeof(message), "execution %zu side", idx);
  require_int(execs[idx].side == side, message);
  snprintf(message, sizeof(message), "execution %zu price", idx);
  require_int(execs[idx].price == price, message);
  snprintf(message, sizeof(message), "execution %zu size", idx);
  require_int(execs[idx].size == size, message);
}

static void test_non_crossing_orders_rest(void) {
  reset_book();
  limit(order4("AAPL", "B001", 0, 10000, 10));
  limit(order4("AAPL", "S001", 1, 10100, 8));
  require_int(execCount == 0, "non-crossing orders should not execute");
  destroy();
}

static void test_buy_crosses_lowest_ask_first(void) {
  reset_book();
  limit(order4("AAPL", "S101", 1, 10100, 4));
  limit(order4("AAPL", "S100", 1, 10000, 3));
  limit(order4("AAPL", "B105", 0, 10500, 5));

  require_int(execCount == 4, "buy should produce two two-sided trades");
  require_exec(0, "AAPL", "B105", 0, 10500, 3);
  require_exec(1, "AAPL", "S100", 1, 10500, 3);
  require_exec(2, "AAPL", "B105", 0, 10500, 2);
  require_exec(3, "AAPL", "S101", 1, 10500, 2);
  destroy();
}

static void test_sell_crosses_highest_bid_first(void) {
  reset_book();
  limit(order4("MSFT", "B090", 0, 9000, 5));
  limit(order4("MSFT", "B095", 0, 9500, 4));
  limit(order4("MSFT", "S080", 1, 8000, 6));

  require_int(execCount == 4, "sell should produce two two-sided trades");
  require_exec(0, "MSFT", "B095", 0, 8000, 4);
  require_exec(1, "MSFT", "S080", 1, 8000, 4);
  require_exec(2, "MSFT", "B090", 0, 8000, 2);
  require_exec(3, "MSFT", "S080", 1, 8000, 2);
  destroy();
}

static void test_same_price_time_priority(void) {
  reset_book();
  limit(order4("TSLA", "S001", 1, 12000, 2));
  limit(order4("TSLA", "S002", 1, 12000, 2));
  limit(order4("TSLA", "B001", 0, 12000, 3));

  require_int(execCount == 4, "same price FIFO should produce two trades");
  require_exec(0, "TSLA", "B001", 0, 12000, 2);
  require_exec(1, "TSLA", "S001", 1, 12000, 2);
  require_exec(2, "TSLA", "B001", 0, 12000, 1);
  require_exec(3, "TSLA", "S002", 1, 12000, 1);
  destroy();
}

static void test_partial_fill_remainder(void) {
  reset_book();
  limit(order4("NVDA", "S001", 1, 50000, 10));
  limit(order4("NVDA", "B001", 0, 50000, 4));
  limit(order4("NVDA", "B002", 0, 50000, 6));

  require_int(execCount == 4, "resting remainder should be matched later");
  require_exec(0, "NVDA", "B001", 0, 50000, 4);
  require_exec(1, "NVDA", "S001", 1, 50000, 4);
  require_exec(2, "NVDA", "B002", 0, 50000, 6);
  require_exec(3, "NVDA", "S001", 1, 50000, 6);
  destroy();
}

static void test_cancel_skips_order(void) {
  t_orderid cancelled;

  reset_book();
  cancelled = limit(order4("META", "S001", 1, 15000, 7));
  cancel(cancelled);
  limit(order4("META", "B001", 0, 16000, 7));

  require_int(execCount == 0, "cancelled resting order should not execute");
  destroy();
}

int main(void) {
  test_non_crossing_orders_rest();
  test_buy_crosses_lowest_ask_first();
  test_sell_crosses_highest_bid_first();
  test_same_price_time_priority();
  test_partial_fill_remainder();
  test_cancel_skips_order();
  printf("All tests passed (6/6).\n");
  return 0;
}
