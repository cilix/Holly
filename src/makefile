CC = clang
WARNS = -W -Wall -pedantic -Wno-comment -Wno-variadic-macros -Wno-unused-function

all:
	$(CC) main.c holly.c $(WARNS) -O3 -o holly -std=c89

test:
	./holly test.txt

clean:
	rm holly