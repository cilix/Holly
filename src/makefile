WARNS = -W -Wall -pedantic -Wno-comment -Wno-variadic-macros -Wno-unused-function

all:
	gcc main.c holly.c $(WARNS) -std=c89 -O3 -lm -o holly 

test:
	./holly test.txt

clean:
	rm -rf holly