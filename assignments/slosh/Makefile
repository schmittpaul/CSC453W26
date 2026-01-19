CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=c99 -D_XOPEN_SOURCE=700
TARGETS = slosh 

all: $(TARGETS)

slosh: slosh.c
	$(CC) $(CFLAGS) -o slosh slosh.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean