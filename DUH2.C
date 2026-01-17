/********************************************************************
* duh2.c - An extremely dumb terminal program (Interrupt Version)
*          Copyright (c) 1992 By Mark Goodwin
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include "serial.h"

#define PORT 2						/* serial port */
#define BAUDRATE 2400				/* baud rate */

void main(void)
{
	int c;

	printf("Duh No. 2 - An Extremely Dumb Terminal Program\n");
	printf("Copyright (c) 1992 By Mark Goodwin\n\n");

	/* open the serial port */
	open_port(PORT, 1024);
	set_port(BAUDRATE, 8, NO_PARITY, 1);

	/* main program loop */
	while (TRUE) {
		/* process keyboard presses */
		if (kbhit()) {
			c = getch();
			switch (c) {
				case 0:						/* exit on Alt-X */
					if (getch() == 45) {
						close_port();
						exit(0);
					}
					break;
				default:
					put_serial(c);			/* send to the serial port */
			}
		}
		/* process remote characters */
		if (in_ready()) {
			c = get_serial();				/* get the character */
			putch(c);						/* display it */
		}
	}
}
