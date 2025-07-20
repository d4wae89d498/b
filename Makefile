CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -fno-pie -no-pie -m32 -ldl #-fsanitize=address -g

SRC=b.c
X86=targets/x86/b2as.c
AS_JIT=targets/x86/as_jit.c
OUT=b

all: $(OUT)

$(OUT): $(SRC) $(X86) $(AS_JIT) b.h
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(AS_JIT)

clean:
	rm -f $(OUT) tests/*.out tests/*.s

.PHONY: test

test: $(OUT)
	chmod +x test_runner.sh
	./test_runner.sh

re: clean all