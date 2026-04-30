#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define UNROLL_FIELD_COPY */
#define MAX_NUM_ORDERS 1010000u

#ifdef DEBUG
#define ASSERT(c)                                                               \
  do {                                                                          \
    if (!(c)) {                                                                 \
      fprintf(stderr, "ASSERT failure at line %d\n", __LINE__);                 \
      exit(1);                                                                  \
    }                                                                           \
  } while (0)
#else
#define ASSERT(c)                                                               \
  do {                                                                          \
  } while (0)
#endif

#ifdef UNROLL_FIELD_COPY
#define COPY_FIELD(dst, src)                                                     \
  do {                                                                          \
    (dst)[0] = (src)[0];                                                         \
    (dst)[1] = (src)[1];                                                         \
    (dst)[2] = (src)[2];                                                         \
    (dst)[3] = (src)[3];                                                         \
    (dst)[4] = '\0';                                                            \
  } while (0)
#else
#define COPY_FIELD(dst, src)                                                     \
  do {                                                                          \
    memcpy((dst), (src), FIELD_CHARS);                                           \
    (dst)[FIELD_CHARS] = '\0';                                                   \
  } while (0)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE static inline __attribute__((always_inline))
#define CACHE_ALIGNED __attribute__((aligned(64)))
#else
#define FORCE_INLINE static inline
#define CACHE_ALIGNED
#endif

typedef struct orderBookEntry {
  t_size size;
  struct orderBookEntry *next;
  char trader[FIELD_LEN];
} orderBookEntry_t;

typedef struct pricePoint {
  orderBookEntry_t *listHead;
  orderBookEntry_t *listTail;
} pricePoint_t;

static pricePoint_t pricePoints[MAX_PRICE + 1] CACHE_ALIGNED;
static orderBookEntry_t arenaBookEntries[MAX_NUM_ORDERS] CACHE_ALIGNED;

static t_orderid curOrderID;
static unsigned int askMin;
static unsigned int bidMax;

void init(void) {
  memset(pricePoints, 0, sizeof(pricePoints));
  memset(arenaBookEntries, 0, sizeof(arenaBookEntries));

  curOrderID = 0;
  askMin = MAX_PRICE + 1;
  bidMax = MIN_PRICE - 1;
}

void destroy(void) {}

FORCE_INLINE void ppInsertOrder(pricePoint_t *ppEntry, orderBookEntry_t *entry) {
  entry->next = NULL;
  if (ppEntry->listHead != NULL) {
    ppEntry->listTail->next = entry;
  } else {
    ppEntry->listHead = entry;
  }
  ppEntry->listTail = entry;
}

FORCE_INLINE void EXECUTE_TRADE(const char *symbol, const char *buyTrader,
                                const char *sellTrader, t_price tradePrice,
                                t_size tradeSize) {
  t_execution exec;

  if (tradeSize == 0) {
    return;
  }

  COPY_FIELD(exec.symbol, symbol);
  exec.price = tradePrice;
  exec.size = tradeSize;

  exec.side = 0;
  COPY_FIELD(exec.trader, buyTrader);
  execution(exec);

  exec.side = 1;
  COPY_FIELD(exec.trader, sellTrader);
  execution(exec);
}

t_orderid limit(t_order order) {
  orderBookEntry_t *bookEntry;
  orderBookEntry_t *entry;
  pricePoint_t *ppEntry;
  t_price price = order.price;
  t_size orderSize = order.size;

  ASSERT(price >= MIN_PRICE && price <= MAX_PRICE);
  ASSERT(orderSize > 0);
  ASSERT(curOrderID + 1 < MAX_NUM_ORDERS);

  if (order.side == 0) {
    if (price >= askMin) {
      ppEntry = pricePoints + askMin;
      do {
        bookEntry = ppEntry->listHead;
        while (bookEntry != NULL) {
          if (bookEntry->size < orderSize) {
            EXECUTE_TRADE(order.symbol, order.trader, bookEntry->trader, price,
                          bookEntry->size);
            orderSize -= bookEntry->size;
            bookEntry = bookEntry->next;
          } else {
            EXECUTE_TRADE(order.symbol, order.trader, bookEntry->trader, price,
                          orderSize);
            if (bookEntry->size > orderSize) {
              bookEntry->size -= orderSize;
            } else {
              bookEntry = bookEntry->next;
            }

            ppEntry->listHead = bookEntry;
            return ++curOrderID;
          }
        }

        ppEntry->listHead = NULL;
        ppEntry++;
        askMin++;
      } while (price >= askMin);
    }

    entry = arenaBookEntries + (++curOrderID);
    entry->size = orderSize;
    COPY_FIELD(entry->trader, order.trader);
    ppInsertOrder(&pricePoints[price], entry);
    if (bidMax < price) {
      bidMax = price;
    }
    return curOrderID;
  }

  if (price <= bidMax) {
    ppEntry = pricePoints + bidMax;
    do {
      bookEntry = ppEntry->listHead;
      while (bookEntry != NULL) {
        if (bookEntry->size < orderSize) {
          EXECUTE_TRADE(order.symbol, bookEntry->trader, order.trader, price,
                        bookEntry->size);
          orderSize -= bookEntry->size;
          bookEntry = bookEntry->next;
        } else {
          EXECUTE_TRADE(order.symbol, bookEntry->trader, order.trader, price,
                        orderSize);
          if (bookEntry->size > orderSize) {
            bookEntry->size -= orderSize;
          } else {
            bookEntry = bookEntry->next;
          }

          ppEntry->listHead = bookEntry;
          return ++curOrderID;
        }
      }

      ppEntry->listHead = NULL;
      ppEntry--;
      bidMax--;
    } while (price <= bidMax);
  }

  entry = arenaBookEntries + (++curOrderID);
  entry->size = orderSize;
  COPY_FIELD(entry->trader, order.trader);
  ppInsertOrder(&pricePoints[price], entry);
  if (askMin > price) {
    askMin = price;
  }
  return curOrderID;
}

void cancel(t_orderid orderid) {
  arenaBookEntries[orderid].size = 0;
}
