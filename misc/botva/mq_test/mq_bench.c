#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


typedef struct {
	long mtype;
	unsigned long long ofs;
	double val;
} msg_t;


int main (int argc, char** argv)
{
	const char* fname;
	int listener = 0;
	key_t ipckey;
	int mq_id;
	msg_t msg;
	unsigned long long count;
	struct timeval tv, tv2;

	if (argc != 3) {
		printf ("Usage: mq_bench -l|-s file\n\n"
			"\t-l starts listener which processes data from the queue\n"
			"\t-s starts seeder which populate queue with some random data\n"
			"\tfile is needed to make queue ID, this must be real file, common to both processes\n");
		return 0;
	}

	fname = argv[2];
	listener = strcmp (argv[1], "-l") == 0;

	ipckey = ftok (fname, 0);

	if (ipckey < 0) {
		printf ("Error creating token: %s\n", strerror (errno));
		return 1;
	}

	mq_id = msgget (ipckey, IPC_CREAT | 0666);

	if (mq_id < 0) {
		printf ("Error creating queue: %s\n", strerror (errno));
		return 1;
	}

	printf ("Starting in %s mode\n", listener ? "listener" : "seeder");

	if (listener) {
		count = 0;
		gettimeofday (&tv, NULL);

		while (1) {
			msgrcv (mq_id, &msg, sizeof (msg), 0, 0);
 			count++;
			gettimeofday (&tv2, NULL);
			if (tv2.tv_sec != tv.tv_sec && tv2.tv_usec > tv.tv_usec) {
				printf ("%lld msgs/s\n", count);
				tv.tv_sec = tv2.tv_sec;
				tv.tv_usec = tv2.tv_usec;
				count = 0;
			}
		}
	}
	else {
		msg.mtype = 1;
		msg.ofs = 0;
		msg.val = 1.0;

		count = 0;
		gettimeofday (&tv, NULL);

		while (1) {
			msgsnd (mq_id, &msg, sizeof (msg), 0);
 			count++;
			gettimeofday (&tv2, NULL);
			if (tv2.tv_sec != tv.tv_sec && tv2.tv_usec > tv.tv_usec) {
				printf ("%lld msgs/s\n", count);
				tv.tv_sec = tv2.tv_sec;
				tv.tv_usec = tv2.tv_usec;
				count = 0;
			}
		}
	}

	return 0;
}
