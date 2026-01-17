/********************************************************************
* duh1.c - An extremely dumb terminal program (ROM BIOS Version)
*          Copyright (c) 1992 By Mark Goodwin
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#define TRUE 1
#define FALSE 0

#define PORT 2						/* serial port */
#define BAUDRATE 2400				/* baud rate */

/* function prototypes */
void open_port(int n, int b);
void put_serial(int n, int c);
int get_serial(int n);
int in_ready(int n);

void main(void)
{
	int c;

	printf("Duh No. 1 - An Extremely Dumb Terminal Program\n");
	printf("Copyright (c) 1992 By Mark Goodwin\n\n");

	/* open the serial port */
	open_port(PORT, BAUDRATE);

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
					put_serial(PORT, c);	/* send to the serial port */
			}
		}
		/* process remote characters */
		if (in_ready(PORT)) {
			/* get character from serial port*/
			c = get_serial(PORT);
			/* display it if one was available */
			if (c != EOF)
				putch(c);
		}
	}
}

/* open serial port using ROM BIOS routine */
void open_port(int n, int b)
{
	union REGS regs;

	/* load AH with the function code */
	regs.h.ah = 0x00;
	/* load AL with the baud rate and 8-N-1 */
	switch (b) {
		case 9600:
			regs.h.al = 0xe3;
			break;
		case 4800:
			regs.h.al = 0xc3;
			break;
		case 2400:
			regs.h.al = 0xa3;
			break;
		case 1200:
			regs.h.al = 0x83;
			break;
		case 300:
			regs.h.al = 0x43;
			break;
		case 150:
			regs.h.al = 0x23;
			break;
		default:
			regs.h.al = 0x03;
	}
	/* load DX with the port number */
	if (n == 1)
		regs.x.dx = 0;
	else
		regs.x.dx = 1;
	/* call the ROM BIOS routine */
	int86(0x14, &regs, &regs);
}

/* put character to serial port */
void put_serial(int n, int c)
{
	union REGS regs;

	/* load AH with the function code */
	regs.h.ah = 0x01;
	/* load AL with the character */
	regs.h.al = c;
	/* load DX with the port number */
	if (n == 1)
		regs.x.dx = 0;
	else
		regs.x.dx = 1;
	/* call the ROM BIOS routine */
	int86(0x14, &regs, &regs);
}

/* get character from serial port */
int get_serial(int n)
{
	union REGS regs;

	/* load AH with the function code */
	regs.h.ah = 0x02;
	/* load DX with the port number */
	if (n == 1)
		regs.x.dx = 0;
	else
		regs.x.dx = 1;
	/* call the ROM BIOS routine */
	int86(0x14, &regs, &regs);
	/* return EOF if timed out */
	if (regs.h.ah & 0x80)
		return EOF;
	return regs.h.al;
}

/* check to see if a character is ready */
int in_ready(int n)
{
	union REGS regs;

	/* load AH with the function code */
	regs.h.ah = 0x03;
	/* load DX with the port number */
	if (n == 1)
		regs.x.dx = 0;
	else
		regs.x.dx = 1;
	/* call the ROM BIOS routine */
	int86(0x14, &regs, &regs);
	/* check for received data ready */
	if (regs.h.ah & 1)
		return TRUE;
	return FALSE;
}