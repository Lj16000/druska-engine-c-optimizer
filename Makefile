CC ?= cc
CFLAGS ?= -O3 -flto -std=c11 -Wall -Wextra -Wno-unused-parameter -DNDEBUG
LDFLAGS ?= -flto

.PHONY: all clean

all: test score

test: engine.o test.o
	$(CC) $(LDFLAGS) -o $@ engine.o test.o

score: engine.o score.o
	$(CC) $(LDFLAGS) -o $@ engine.o score.o

engine.o: engine.c engine.h
	$(CC) $(CFLAGS) -c engine.c

test.o: test.c engine.h
	$(CC) $(CFLAGS) -c test.c

score.o: score.c engine.h
	$(CC) $(CFLAGS) -c score.c

clean:
	rm -f *.o test score score_output.txt
