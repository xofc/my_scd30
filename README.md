# my_scd30
A quick little 'C' program to check my [Sensirion SCD30](https://sensirion.com/products/catalog/SCD30/) is working (using modbus on a Raspberry Pi)

It has no dependency at all, it just uses Linux open/ioctl/read/write on a Raspberry's /dev/ttyS0 and the modbus messages are copied from the datasheet
[Sensirion_CO2_Sensors_SCD30_Interface_Description.pdf](https://sensirion.com/media/documents/D7CEEF4A/6165372F/Sensirion_CO2_Sensors_SCD30_Interface_Description.pdf)

```
$ gcc -o my_scd30 [my_scd30.c](https://github.com/xofc/my_scd30/blob/main/my_scd30.c)
$ ./my_scd30
test	439.1
CO2:	1022.7
T  :	21.4
RH :	30.7

CO2:	1023.2
T  :	21.3
RH :	30.6

CO2:	1023.8
T  :	21.4
RH :	30.6
...

```

# Remarks
I intended to use i2c but it needs 'clock stretching' annd it does not seem to be that easy on a Raspberry.  After attempting to use programs found on the Internet which had many dependencies and I was not able to use at all, I decided to use modbus instead of i2c.  I first used [mbpoll(1)](https://manpages.ubuntu.com/manpages/impish/man1/mbpoll.1.html) ([git](https://github.com/epsilonrt/mbpoll.git)) and was able to get the firmware version with
```
$ mbpoll -m RTU -a 0x61 -b 19200 -P none -t 4:hex -r 0x0021 -c 1 /dev/ttyS0 -v -1
...
[61][03][00][20][00][01][8C][60]
Waiting for a confirmation...
<61><03><02><03><42><B8><8D>
[33]: 	0x0342
...
```
But was not able to get the CO2 values (It should be possible but I don't know what to do).

# Raspberry Pi configuration
One has to use 'sudo [raspi-config](https://www.raspberrypi.com/documentation/computers/configuration.html)' to enable the serial link (but not the login process on it).  To be able to write on /dev/ttyS0 (gpio pins 8 & 10) as a normal user, it must be member of the 'dialout' group ($ sudo adduser $USER dialout).  A possibility to test the serial link is to use [minicom(1)](https://manpages.ubuntu.com/manpages/impish/man1/minicom.1.html) and to short-cut Rx & Tx.  Setting up minicom is a little bit tricky as, by default, hardware handshaking seems to be enabled.
