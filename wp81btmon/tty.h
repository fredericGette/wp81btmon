/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
*
*  BlueZ - Bluetooth protocol stack for Linux
*
*  Copyright (C) 2016  Intel Corporation. All rights reserved.
*
*
*/

// https://github.com/openbsd/src/blob/master/sys/sys/termios.h
/*
* Standard speeds
*/
#define B0	0
#define B50	50
#define B75	75
#define B110	110
#define B134	134
#define B150	150
#define B200	200
#define B300	300
#define B600	600
#define B1200	1200
#define	B1800	1800
#define B2400	2400
#define B4800	4800
#define B9600	9600
#define B19200	19200
#define B38400	38400
#define B7200	7200
#define B14400	14400
#define B28800	28800
#define B57600	57600
#define B76800	76800
#define B115200	115200
#define B230400	230400
#define EXTA	19200
#define EXTB	38400

static inline unsigned int tty_get_speed(int speed)
{
	switch (speed) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	}

	return 0;
}