#include <aio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>


#define BUF_SIZE (100*1000*1000)

int fin = 0;


void aio_completion_handler (sigval_t sigval)
{
	struct aiocb *req;
	req = (struct aiocb *)sigval.sival_ptr;
	fin--;
	free ((char*)req->aio_buf);
	free (req);
}


int request_data_append (int fd)
{
	struct aiocb* cb;
	unsigned long long ofs = 0;

	cb = calloc (1, sizeof(struct aiocb));
	cb->aio_fildes = fd;
	cb->aio_buf = malloc (BUF_SIZE);
	cb->aio_nbytes = BUF_SIZE;
	cb->aio_offset = fin*BUF_SIZE;

	bzero ((char*)cb->aio_buf, BUF_SIZE);

	cb->aio_sigevent.sigev_notify = SIGEV_THREAD;
	cb->aio_sigevent.sigev_notify_function = aio_completion_handler;
	cb->aio_sigevent.sigev_notify_attributes = NULL;
	cb->aio_sigevent.sigev_value.sival_ptr = cb;

	fin++;

	return aio_write (cb);
}


int main (int argc, char** argv)
{
	int fd, i;

	unlink (argv[1]);

	fd = open (argv[1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

	if (fd < 0) {
		printf ("Cannot open file descriptor!\n");
		return 1;
	}

	for (i = 0; i < 10; i++)
		if (!request_data_append (fd))
			printf ("Write request enqueued\n");

	while (fin) {
		printf ("In infinite loop, fin=%d...\n", fin);
		sleep (1);
	}

	close (fd);
	printf ("Finishing...\n");

	return 0;
}


