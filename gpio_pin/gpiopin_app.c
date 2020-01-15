
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

int main(int argc, int **argv)
{
	int fd;
	uint8_t w_level = 1, r_level = 0;
	
	fd = open("/dev/gp0", O_RDWR);
	if(-1 == fd)
	{
		perror("open gpiopin failed\n");
		return -1;
	}
	printf("open gpiopin driver succ\n");
	
	for(;;)
	{
		write(fd, &w_level, 1);
		read(fd, &r_level, 1);
		printf("pin = %d\n", r_level);
		w_level = !w_level;
		sleep(1);
	}
	close(fd);
	
	return 0;
}

