WARNS = -W -Wall -pedantic -Wno-comment -Wno-variadic-macros -Wno-unused-function

all:
	gcc main.c lex.c parse.c vm.c hash.c util.c $(WARNS) -std=c89 -O3 -o lang

test:
	./lang test.txt

clean:
	rm -rf lang