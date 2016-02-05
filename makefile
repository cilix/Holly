WARNS = -W -Wall -pedantic -Wno-comment -Wno-variadic-macros -Wno-unused-function

all:
	gcc main.c lex.c parse.c vm.c hash.c util.c $(WARNS) -std=c89 -O3 -o lang

holly:
	gcc holly.c $(WARNS) -std=c89 -O3 -o holly

test:
	./lang test2.txt

clean:
	rm -rf lang