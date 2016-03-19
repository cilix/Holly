CC = clang
WARNS = -Wall -ansi -pedantic

all:
	$(CC) main.c holly.c $(WARNS) -O3 -o holly -std=c89

test:
	./holly test.txt

clean:
	rm holly