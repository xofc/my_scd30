#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

#undef DEBUG

void bail_out(char *str)
	{
	}
void xdump(char *label, u_char *ucp, int len)
	{
#ifdef DEBUG
	printf(label);
	while (--len >= 0)
		printf(" 0x%02x", *ucp++);
	printf("\n");
#endif
	}
void conv_dump(char *label, u_char *ucp)
	{
	u_int x;
	float f;

	x = (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;
	x <<= 8;
	x += (u_int) *ucp++;
#ifdef	DEBUG
	printf("0x%0x\n", x);
#endif
	f = *(float*)&x;
	printf("%s\t%.1f\n", label, f);
	}
u_char tst_buf[] = { 0x43, 0xDB, 0x8C, 0x2E };
u_char get_firmware_version[] = { 0x61, 0x03, 0x00, 0x20, 0x00, 0x01, 0x8C, 0x60 };
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
conv_dump("test", tst_buf);
	xdump("Writing:", get_firmware_version, sizeof(get_firmware_version));
	if (write(fd, get_firmware_version, sizeof(get_firmware_version)) != sizeof(get_firmware_version))
		{
		perror(devstrp);
		exit(-1);
		}
	n = read(fd, buf, sizeof(buf));
	xdump("Reading:", buf, n);

for (;;){
	xdump("Writing:", read_measurement, sizeof(read_measurement));
	if (write(fd, read_measurement, sizeof(read_measurement)) != sizeof(read_measurement))
		{
		perror(devstrp);
		exit(-1);
		}
	n = read(fd, buf, sizeof(buf));
	xdump("Reading:", buf, n);
	xdump("CO2:", buf+3, 4);
	xdump("T  :", buf+7, 4);
	xdump("RH :", buf+11, 4);
	conv_dump("CO2:", buf+3);
	conv_dump("T  :", buf+7);
	conv_dump("RH :", buf+11);
	printf("\n");
sleep(2); }
	printf("So far, so good.\n");
	close(fd);
	}
