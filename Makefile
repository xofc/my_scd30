all: my_scd30 influx_scd30

my_scd30 : my_scd30.c
	gcc -o my_scd30 my_scd30.c

influx_scd30 : influx_scd30.c
	gcc -o influx_scd30 influx_scd30.c
