#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>

int main (int argc, char **argv)
{
    unsigned long i, j;
    int fd;
    char *buf, *read_buf, *mmap_buf, *filename = "/dev/asgn1";
    int nproc = 12345;

    srandom (getpid ());

    if (argc > 1)
        filename = argv[1];

    if ((fd = open (filename, O_RDWR)) < 0) {
        fprintf (stderr, "open of %s failed:  %s\n", filename,
                 strerror (errno));
        exit (1);
    }

	fprintf(stderr, "OPEN WORKING, WILL NOW CLOSE");
	
	
	return 0;	
}
