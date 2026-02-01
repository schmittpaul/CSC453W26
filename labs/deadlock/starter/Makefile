CC      := cc
CFLAGS  := -std=c99 -D_POSIX_C_SOURCE=200112L -O2 -Wall -Wextra
LDFLAGS := -pthread

all: bank

bank: bank.c common.h
	$(CC) $(CFLAGS) -o $@ bank.c $(LDFLAGS)

clean:
	rm -f bank

.PHONY: all clean
