#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <getopt.h>
#include <time.h>

int	verbose = 0;

void bail_out(char *str)
	{
	perror(str);
	exit(-1);
	}
void xdump(char *label, u_char *ucp, int len)
	{
	printf(label);
	while (--len >= 0)
		printf(" 0x%02x", *ucp++);
	printf("\n");
	}
uint16_t crc_modbus(u_char *buf, int len)
	{
	uint16_t crc = 0xFFFF;
  
	while (--len >= 0)
		{
		crc ^= (uint16_t)*buf++;
	
		for (int i = 8; i != 0; i--)
			{
			if (crc & 0x0001)
				{
				crc >>= 1;
				crc ^= 0xA001;
				}
			else	crc >>= 1;
			}
		}
	return(crc);	
	}
int x_modbus(int fd, u_char *msg_out, int len_out, u_char *msg_in, int len_in)
	{
	int		n;
	uint16_t	crc;

	crc = crc_modbus(msg_out, len_out);
	msg_out[len_out] = crc & 0xff;
	msg_out[len_out+1] = crc >> 8;
	if (write(fd, msg_out, len_out + 2) != (len_out + 2))
		bail_out("write");
	n = read(fd, msg_in, len_in);
	if (n < 0)
		bail_out("read");
	crc = crc_modbus(msg_in, n - 2);
	if ( ((crc & 0xff) != msg_in[n - 2]) || ((crc >> 8) != msg_in[n-1]) )
		{
		xdump("msg_out ", msg_out, len_out + 2);
		xdump("msg_in ", msg_in, n);
		printf("crc %0xx %02x\n", (crc & 0xff), (crc >> 8));
		return(-1);
		}
	if (verbose)
		{
		xdump("msg_out ", msg_out, len_out + 2);
		xdump("msg_in ", msg_in, n);
		printf("ok crc %0xx %02x\n", (crc & 0xff), (crc >> 8));
		}
	return(n-2);
	}
float s2f(u_char *ucp)
	{
	/* u_char tst_buf[] = { 0x43, 0xDB, 0x8C, 0x2E };	 s2f(tst_buf) = 439.09 (doc p.19/21) */
	uint32_t	x;
	float		f;

	x = (uint32_t) *ucp++;
	x <<= 8;
	x += (uint32_t) *ucp++;
	x <<= 8;
	x += (uint32_t) *ucp++;
	x <<= 8;
	x += (uint32_t) *ucp++;
	f = *(float*)&x;
	return(f);
	}
int open_dev(char *dev_name)
	{
	char	*devstrp = "/dev/ttyS0";
	int	fd, n;
	struct	termios tty;
	u_char	buf[256];

	if ((fd = open(devstrp, O_RDWR)) < 0)
		bail_out(devstrp);

	tty.c_cflag = CLOCAL | CREAD | CS8;
	tty.c_lflag = 0;
	tty.c_iflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 100;
	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		bail_out(devstrp);
	return(fd);
	}
u_char get_firmware_version[] = { 0x61, 0x03, 0x00, 0x20, 0x00, 0x01, 0x8C, 0x60 };
u_char read_measurement[] = { 0x61, 0x03, 0x00, 0x28, 0x00, 0x06, 0x4C, 0x60 };

int main(int argc, char *argv[])
	{
	char	*devstrp = "/dev/ttyS0";
	int	fd, n;
	u_char	buf[256];
	int	now;

	if ((fd = open_dev(devstrp)) < 0)
		bail_out(devstrp);

	if ((n = x_modbus(fd, get_firmware_version, 6, buf, sizeof(buf))) < 0)
		bail_out("something bad");

	for (;;){
		now = time(0);
		if (x_modbus(fd, read_measurement, 6, buf, sizeof(buf)) > 0)
			printf("scd30,tag=ok co2=%.1f,t=%.1f,rh=%.1f %d000000000\n", s2f(buf+3), s2f(buf+7), s2f(buf+11), now);
		else	printf("scd30,tag=ko co2=0.0,t=0.0,rh=0.0 %d000000000\n", now);
		sleep(10);
		}
	printf("So far, so good.\n");
	close(fd);
	}
