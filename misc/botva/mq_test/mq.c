#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <errno.h>


typedef struct {
	long mtype;
	char buf[256];
} msg_t;


int main ()
{
	/* trying to push as much messages into queue as possible */
	key_t ipckey;
	int mq_id;
	msg_t msg;
	int succ = 0;
	int id = 0;

	while (1) {
		ipckey = ftok ("/tmp", id++);

		if (ipckey < 0) {
			printf ("Error creating token: %s\n", strerror (errno));
			return 1;
		}

		//		printf ("IPC key: %d\n", ipckey);

		mq_id = msgget (ipckey, IPC_CREAT | 0666);
		//		printf ("MQ id: %d\n", mq_id);

		if (mq_id < 0)
			return 1;

		msg.mtype = 1;
		succ = 0;

		while (1) {
			if (msgsnd (mq_id, &msg, sizeof (msg), IPC_NOWAIT) < 0) {
				printf ("Error adding %d'th message: %s\n", succ, strerror (errno));
				break;
			}
			else {
				succ++;
				//				printf ("Added %d messages\n", succ);
			}
		}

		printf ("Added %d messages\n", succ);
	}

	return 0;
}
