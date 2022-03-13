#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>

void bail_out(char *str)
	{
	}
float s2f(u_char *ucp)
	{
	u_int x;

	x = (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;

	return(*(float*)&x);
	}

u_char read_measurement[] = { 0x61, 0x03, 0x00, 0x28, 0x00, 0x06, 0x4C, 0x60 };

int main(int argc, char *argv[])
	{
	char	*devstrp = "/dev/ttyS0";
	int	fd, n;
	struct	termios tty;
	u_char	buf[256];

	if ((fd = open(devstrp, O_RDWR)) < 0)
		{
		perror(devstrp);
		exit(-1);
		}
	tty.c_cflag = CLOCAL | CREAD | CS8;
	tty.c_lflag = 0;
	tty.c_iflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 100;
	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		{
		perror(devstrp);
		exit(-1);
		}

	for (;;){
		if (write(fd, read_measurement, sizeof(read_measurement)) != sizeof(read_measurement))
			{
			perror(devstrp);
			exit(-1);
			}
		n = read(fd, buf, sizeof(buf));
		printf("scd30,tag=val co2=%.1f,t=%.1f,rh=%.1f %d%s\n", s2f(buf+3), s2f(buf+7), s2f(buf+11), time(NULL), "000000000");
		sleep(10);
		}
	}
