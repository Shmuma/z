#include <stdio.h>
#include <stdlib.h>


typedef union {
	unsigned long long i;
	double f;
} val_t;


int main (int argc, char** argv)
{
	FILE* f = fopen (argv[1], "rb");
	val_t v;
	int ofs = 0;

	if (!f) {
		printf ("File not found!\n");
		return 1;
	}

	while (fread (&v, sizeof (v), 1, f) == 1)
		printf ("%08x (%6d): %20llu, %016llx, %20f\n", ++ofs, ofs, v.i, v.i, v.f);

	fclose (f);

	return 0;
}
