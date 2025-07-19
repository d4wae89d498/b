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

test: $(OUT) test.b
	./$(OUT) -S test.b > out.s
	@echo "Assembly written to out.s" 

re: clean all