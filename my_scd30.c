#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <getopt.h>
#include <time.h>

/*
** global options
*/
int	temperature = -1;
int	altitude = -1;
int	dtime = -1;
int	sleep_time = 10;
int	ppm = -1;
int	verbose = 0;
int	query = 0;
int	continuous = 0;

void usage()
	{
	fprintf(stderr, "my_scd30 : handle a Sensirion SCD30 though the modbus protocol\n");
	fprintf(stderr, "my_scd30 [--device <dev>] [--verbose] [--altitude <altitude>] [--ppm <ppm>] [--temperature] [--query] [--continuous] [--measure <sleep_time>]\n");
	fprintf(stderr, "\t-d --device <dev> (default = /dev/ttyS0)\n");
	fprintf(stderr, "\t-v --verbose (default = no)\n");
	fprintf(stderr, "\t-a --altitude <altitude> (in meters above sea level)\n");
	fprintf(stderr, "\t-p --ppm <ppm0> (force current ppm value)\n");
	fprintf(stderr, "\t-T --temperature <degrees> (delta temperature)\n");
	fprintf(stderr, "\t-q --query (read registers values)\n");
	fprintf(stderr, "\t-c --continuous <dtime>\n");
	fprintf(stderr, "\t-m --measure <sleep_time>\n");
	
	exit(-1);
	}

void bail_out(char *str)
	{
	perror(str);
	exit(-1);
	}
int a2i(char *str)	/* where is it gone ?!? */
	{
	int	sum = 0;

	while (*str)
		sum = (sum * 10) + (*str++) - '0';
	return(sum);
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
	int	fd, n;
	struct	termios tty;
	u_char	buf[256];

	if ((fd = open(dev_name, O_RDWR)) < 0)
		bail_out(dev_name);

	tty.c_cflag = CLOCAL | CREAD | CS8;
	tty.c_lflag = 0;
	tty.c_iflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 100;
	cfsetispeed(&tty, B19200);
	cfsetospeed(&tty, B19200);
	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		bail_out(dev_name);
	return(fd);
	}
u_char get_firmware_version[] = { 0x61, 0x03, 0x00, 0x20, 0x00, 0x01, 0x8C, 0x60 };
u_char read_measurement[] = { 0x61, 0x03, 0x00, 0x28, 0x00, 0x06, 0x4C, 0x60 };

int main(int argc, char *argv[])
	{
	char	*devstrp = "/dev/ttyS0";	/* default tty on Raspberry Pi */
	int	fd, n;
	u_char	buf[256];
	int	now;
	int c;
	int digit_optind = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] =
			{
			{"device",	required_argument,	0, 'd' },
			{"verbose",	no_argument,		0, 'v' },
			{"altitude",	required_argument,	0, 'a' },
			{"ppm",		required_argument,	0, 'p' },
			{"temperature",	required_argument,	0, 'T' },
			{"query",	no_argument,		0, 'q' },
			{"continuous",	required_argument,	0, 'c' },
			{"measure",	required_argument,	0, 'm' },
			{0,		0,			0,  0 }
			};

		if ((c = getopt_long(argc, argv, "d:va:p:Tc:m:?", long_options, &option_index)) == -1)
			break;

		switch (c) {
		case 'd':	/* device */
			devstrp = optarg;
			break;

		case 'v':	/* verbose */
			verbose = 1;
			break;

		case 'a':	/* altitude */
			altitude = a2i(optarg);
			break;

		case 'p':	/* ppm CO2 */
			ppm = a2i(optarg);
			break;

		case 'T':	/* Temperature */
			temperature = a2i(optarg);
			break;

		case 'c':	/* continuous measurement */
			dtime = a2i(optarg);
			break;

		case 'm':	/* measure */
			sleep_time = a2i(optarg);
			break;

		case '?':
			usage();
			break;

		default:
			fprintf(stderr, "Unknown option '%c'\n", c);
			usage();
		}
	}

	if (optind < argc)
		{
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
		}
	fprintf(stderr, "---- Not all parameters effective yet ----\n");
	fprintf(stderr, "device\t%s\n", devstrp);
	fprintf(stderr, "Temperature\t%d\n", temperature);
	fprintf(stderr, "altitude\t%d\n", altitude);
	fprintf(stderr, "dtime\t%d\n", dtime);
	fprintf(stderr, "ppm\t%d\n", ppm);
	fprintf(stderr, "verbose\t%d\n", verbose);
	fprintf(stderr, "query\t%d\n", query);
	fprintf(stderr, "continuous\t%d\n", continuous);
	fprintf(stderr, "sleep_time\t%d\n", sleep_time);
	fprintf(stderr, "----\n");

	if ((fd = open_dev(devstrp)) < 0)
		bail_out(devstrp);

	if ((n = x_modbus(fd, get_firmware_version, 6, buf, sizeof(buf))) < 0)
		bail_out("something bad");

	for (;;){
		now = time(0);
		if (x_modbus(fd, read_measurement, 6, buf, 17) > 0)
			printf("scd30,tag=ok co2=%.1f,t=%.1f,rh=%.1f %d000000000\n", s2f(buf+3), s2f(buf+7), s2f(buf+11), now);
		else	printf("scd30,tag=ko co2=0.0,t=0.0,rh=0.0 %d000000000\n", now);
		sleep(sleep_time);
		}
	printf("So far, so good.\n");
	close(fd);
	}
