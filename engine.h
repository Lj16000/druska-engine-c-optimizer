#pragma once

#include <stddef.h>

#define MIN_PRICE 1u
#define MAX_PRICE 65536u
#define FIELD_CHARS 4
#define FIELD_LEN (FIELD_CHARS + 1)

typedef unsigned int t_orderid;
typedef unsigned int t_price;
typedef unsigned long t_size;
typedef int t_side;

/* side: 0 = buy, 1 = sell */
typedef struct {
  char symbol[FIELD_LEN];
  char trader[FIELD_LEN];
  t_side side;
  t_price price;
  t_size size;
} t_order;

typedef t_order t_execution;

void init(void);
void destroy(void);
t_orderid limit(t_order order);
void cancel(t_orderid orderid);

/* Implemented by the harness. Called once for each side of a trade. */
void execution(t_execution exec);
