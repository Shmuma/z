all: mq mq_bench

mq: mq.c
	gcc -g -O0 -o mq mq.c

mq_bench: mq_bench.c
	gcc -g -O0 -o mq_bench mq_bench.c

clean:
	rm -f mq mq_bench