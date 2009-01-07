#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BUF_SIZE (100*1000*1000)


int main (int argc, char** argv)
{
	int fd;
	int res;
	char* buf;

	unlink (argv[1]);
	fd = open (argv[1], O_ASYNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	buf = (char*)calloc (1, BUF_SIZE);
	
	printf ("%u: Before write\n", time (NULL));
	res = write (fd, buf, BUF_SIZE);
	printf ("%u: After write (%d)\n", time (NULL), res);
	close (fd);
	printf ("%u: After close\n", time (NULL));

	return 0;
}
