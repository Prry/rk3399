
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h> 
#include <sys/ioctl.h>

int fd;

void gq_sig_handle(int sig)
{
	char level = 0;
	
	read(fd, &level, 1);
	
	printf("IO = %d\n", level);
}


int main(int argc, int **argv)
{
	int flags;
	
	fd = open("/dev/gq0", O_RDWR);
	if(-1 == fd)
	{
		perror("open gq0 failed\n");
		return -1;
	}
	printf("open gq0 driver succ\n");

	if (SIG_ERR == signal(SIGIO, gq_sig_handle))
    {
        perror("signal error\n");
        return -1;
    }
	fcntl(fd, F_SETOWN, getpid());
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_ASYNC);
	
	for(;;)
	{
		sleep(1);
	}
	close(fd);
	
	return 0;
}

