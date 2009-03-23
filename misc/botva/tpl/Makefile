all: test test2

test: test.c
	gcc -o test -I../../../include/ test.c tpl.c -g -O0

test2: test2.c
	gcc -o test2 -I../../../include/ test2.c tpl.c -g -O0

clean:
	rm -f test test2