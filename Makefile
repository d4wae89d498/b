CC=gcc
CFLAGS=-std=c99 -Wall -Wextra

SRC=b.c
X86=targets/x86.c
OUT=b

all: $(OUT)

$(OUT): $(SRC) $(X86) b.h
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT) out.s

.PHONY: test

test: $(OUT)
	chmod +x test_runner.sh
	./test_runner.sh

re: clean all