/********************************************************************
* duh3.cpp - An extremely dumb terminal program (C++ Interrupt Version)
*            Copyright (c) 1992 By Mark Goodwin
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "sercpp.h"

#define PORT 2						/* serial port */
#define BAUDRATE 2400				/* baud rate */

SERIALPORT port(PORT, 1024);

void main(void)
{
	int c;

	printf("Duh No. 3 - An Extremely Dumb Terminal Program\n");
	printf("Copyright (c) 1992 By Mark Goodwin\n\n");

	/* open the serial port */
	port.set_port(BAUDRATE);

	/* main program loop */
	while (TRUE) {
		/* process keyboard presses */
		if (kbhit()) {
			c = getch();
			switch (c) {
				case 0:						/* exit on Alt-X */
					if (getch() == 45)
						exit(0);
					break;
				default:
					port.put(c);			/* send to the serial port */
			}
		}
		/* process remote characters */
		if (port.in_ready()) {
			c = port.get();				/* get the character */
			putch(c);						/* display it */
		}
	}
}
