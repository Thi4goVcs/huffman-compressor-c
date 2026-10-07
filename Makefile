CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

all: huffman

huffman: src/main.c src/huffman.c src/huffman.h
	$(CC) $(CFLAGS) -o huffman src/main.c src/huffman.c

test_huffman: tests/test_huffman.c src/huffman.c src/huffman.h
	$(CC) $(CFLAGS) -o test_huffman tests/test_huffman.c src/huffman.c

test: test_huffman
	./test_huffman

clean:
	rm -f huffman huffman.exe test_huffman test_huffman.exe

.PHONY: all test clean
