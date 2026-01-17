/**********************************************************************
* serial.c - Low-Level Serial Communications Routines                 *
*            Copyright (c) 1992 By Mark D. Goodwin                    *
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#include <time.h>
#include "serial.h"

#ifndef __TURBOC__
#define enable _enable
#define inportb inp
#define outportb outp
#define getvect _dos_getvect
#define disable _disable
#define setvect _dos_setvect
#define farmalloc _fmalloc
#endif

/* UART register constants */
#define TX 0							/* transmit register */
#define RX 0							/* receive register */
#define DLL 0							/* divisor latch low */
#define IER 1							/* interrupt enable register */
#define DLH 1							/* divisor latch high */
#define IIR 2							/* interrupt id register */
#define LCR 3							/* line control register */
#define MCR 4							/* modem control register */
#define LSR 5							/* line status register */
#define MSR 6							/* modem status register */

/* interrupt enable register constants */
#define RX_INT 1						/* received data bit mask */

/* interrupt id register constants */
#define INT_MASK 7						/* interrupt mask */
#define RX_ID 4							/* received data interrupt */

/* line control register constants */
#define DLAB 0x80						/* DLAB bit mask */

/* modem control register constants */
#define DTR 1							/* Date Terminal Ready bit mask */
#define RTS 2							/* Request To Send bit mask */
#define MC_INT 8						/* GPO2 bit mask */

/* line status register constants */
#define RX_RDY 0x01
#define TX_RDY 0x20

/* modem status register constants */
#define CTS 0x10
#define DSR 0x20
#define DCD 0x80

/* 8259 PIC constants */
#define IMR 0x21						/* interrupt mask register */
#define ICR 0x20						/* interrupt control register */

/* interrupt mask register constants */
#define IRQ3 0xf7						/* IRQ 3 interrupt mask */
#define IRQ4 0xef						/* IRQ 4 interrupt mask */

/* interrupt control register constants */
#define EOI 0x20						/* end of interrupt mask */

/* XON/XOFF flow control constants */
#define XON 0x11
#define XOFF 0x13

int port_open = FALSE, irq;
#ifdef __TURBOC__
void interrupt (*oldvect)();
#else
void (_interrupt _far *oldvect)();
#endif

/* Check for a Port's Existence */
int port_exist(int port)
{
	return mpeek(0, 0x400 + (port - 1) * 2);
}

/* Open a Serial Port */
void open_port(int port, int inlen)
{
	long i;

	/* make sure a port isn't already open */
	if (port_open) {
		printf("Unable to open port: A port is already open!\n");
		exit(1);
	}

	/* make sure the port if valid */
	if (port < 1 || port > 4 || !port_exist(port)) {
		printf("Unable to open port: Invalid port number!\n");
		exit(1);
	}

	/* allocate the space for the buffers */
	ilen = inlen;
	if ((inbuff = farmalloc(ilen)) == NULL) {
		printf("Unable to open port: Not enough memory for the buffer!\n");
		exit(1);
	}

	/* calculate the flow control limits */
	foff = (int)((long)ilen * 90L / 100L);
	fon = (int)((long)ilen * 80L / 100L);
	rx_flow = FALSE;

	/* set the base address and IRQ*/
	switch (port) {
		case 1:
			base = 0x3f8;
			irq = 0x0c;
			break;
		case 2:
			base = 0x2f8;
			irq = 0x0b;
			break;
		case 3:
			base = 0x3e8;
			irq = 0x0c;
			break;
		case 4:
			base = 0x2e8;
			irq = 0x0b;
			break;
	}

	/* set up the interrupt handler */
	disable();                         		/* disable the interupts */
	oldvect = getvect(irq);					/* save the current vector */
	setvect(irq, handler);					/* set the new vector */
	sibuff = eibuff = 0;					/* set the buffer pointers */
	outportb(base + MCR,
		inportb(base + MCR) | MC_INT | DTR | RTS); /* assert GPO2, RTS, and DTR */
	outportb(base + IER, RX_INT);			/* set received data interrupt */
	outportb(IMR,
		inportb(IMR) & (irq == 0x0b ? IRQ3 : IRQ4)); /* set the interrupt */
	enable();								/* enable the interrupts */
	fifo(14);								/* set FIFO buffer for 14 bytes */
	set_tx_rts(FALSE);						/* turn off RTS/CTS flow control */
	set_tx_dtr(FALSE);						/* turn off DTR/DSR flow control */
	set_tx_xon(FALSE);						/* turn off XON/XOFF flow control */
	set_rx_rts(FALSE);						/* turn off RTS/CTS flow control */
	set_rx_dtr(FALSE);						/* turn off DTR/DSR flow control */
	set_rx_xon(FALSE);						/* turn off XON/XOFF flow control */
	port_open = TRUE;						/* flag port is open */
}

/* close serial port routine */
void close_port(void)
{
	/* check to see if a port is open */
	if (!port_open)
		return;

	port_open = FALSE;						/* flag port not opened */

	disable();								/* disable the interrupts */
	outportb(IMR,
		inportb(IMR) | (irq == 0x0b ? ~IRQ3 : ~IRQ4)); /* clear the interrupt */
	outportb(base + IER, 0);				/* clear recceived data int */
	outportb(base + MCR,
		inportb(base + MCR) & ~MC_INT);		/* unassert GPO2 */
	setvect(irq, oldvect);					/* reset the old int vector */
	enable();								/* enable the interrupts */
	outportb(base + MCR,
		inportb(base + MCR) & ~RTS);		/* unassert RTS */
}

/* purge input buffer */
void purge_in(void)
{
	disable();                         		/* disable the interrupts */
	sibuff = eibuff = 0;					/* set the buffer pointers */
	enable();								/* enable the interrupts */
}

/* set the baud rate */
void set_baud(long baud)
{
	int c, n;

	/* check for 0 baud */
	if (baud == 0L)
		return;
	n = (int)(115200L / baud);				/* figure the divisor */
	disable();								/* disable the interrupts */
	c = inportb(base + LCR);				/* get line control reg */
	outportb(base + LCR, (c | DLAB));		/* set divisor latch bit */
	outportb(base + DLL, n & 0x00ff);		/* set LSB of divisor latch */
	outportb(base + DLH, (n >> 8) & 0x00ff); /* set MSB of divisor latch */
	outportb(base + LCR, c);				/* restore line control reg */
	enable();								/* enable the interrupts */
}

/* get the baud rate */
long get_baud(void)
{
	int c, n;

	disable();								/* disable the interrupts */
	c = inportb(base + LCR);				/* get line control reg */
	outportb(base + LCR, (c | DLAB));		/* set divisor latch bit */
	n = inportb(base + DLH) << 8;			/* get MSB of divisor latch */
	n |= inportb(base + DLL);				/* get LSB of divisor latch */
	outportb(base + LCR, c);				/* restore the line control reg */
	enable();								/* enable the interrupts */
	return 115200L / (long)n;				/* return the baud rate */
}

/* get number of data bits */
int get_bits(void)
{
	return (inportb(base + LCR) & 3) + 5;		/* return number of data bits */
}

/* get parity */
int get_parity(void)
{
	switch ((inportb(base + LCR) >> 3) & 7) {
		case 0:
			return NO_PARITY;
		case 1:
			return ODD_PARITY;
		case 3:
			return EVEN_PARITY;
	}
	return -1;
}

/* get number of stop bits */
int get_stopbits(void)
{
	if (inportb(base + LCR) & 4)
		return 2;
	return 1;
}

void set_data_format(int bits, int parity, int stopbit)
{
	int n;

	/* check parity value */
	if (parity < NO_PARITY || parity > ODD_PARITY)
		return;

	/* check number of bits */
	if (bits < 5 || bits > 8)
		return;

	/* check number of stop bits */
	if (stopbit < 1 || stopbit > 2)
		return;

	n = bits - 5;							/* figure the bits value */
	n |= ((stopbit == 1) ? 0 : 4);			/* figure the stop bit value */

	/* figure the parity value */
	switch (parity) {
		case NO_PARITY:
			break;
		case ODD_PARITY:
			n |= 0x08;
			break;
		case EVEN_PARITY:
			n |= 0x18;
			break;
	}

	disable();								/* disable the interrupts */
	outportb(base + LCR, n);				/* set the port */
	enable();								/* enable the interrupts */
}

void set_port(long baud, int bits, int parity, int stopbit)
{
	/* check for open port */
	if (!port_open)
		return;

	set_baud(baud);							/* set the baud rate */
	set_data_format(bits, parity, stopbit); /* set the data format */
}

/* check for byte in input buffer */
int in_ready(void)
{
	return !(sibuff == eibuff);
}

/* check for carrier routine */
int carrier(void)
{
	return inportb(base + MSR) & DCD ? TRUE : FALSE;
}

/* set DTR routine */
void set_dtr(int n)
{
	if (n)
		outportb(base + MCR, inportb(base + MCR) | DTR); /* assert DTR */
	else
		outportb(base + MCR, inportb(base + MCR) & ~DTR); /* unassert RTS */
}

/* 16550 FIFO routine */
void fifo(int n)
{
	int i;

	switch (n) {
		case 1:
			i = 1;							/* 1 byte FIFO buffer */
			break;
		case 4:
			i = 0x41;						/* 4 byte FIFO buffer */
			break;
		case 8:
			i = 0x81;						/* 8 byte FIFO buffer */
			break;
		case 14:
			i = 0xc1;						/* 14 byte FIFO buffer */
			break;
		default:
			i = 0;							/* turn FIFO off for all others */
	}
	outportb(base + IIR, i);				/* set the FIFO buffer */
}

#ifndef __TURBOC__
void delay(clock_t n)
{
	clock_t i;

	i = n + clock();
	while (i > clock()) ;
}
#endif

/* set transmit RTS/CTS flag */
void set_tx_rts(int n)
{
	tx_rts = n;
}

/* set transmit DTR/DSR flag */
void set_tx_dtr(int n)
{
	tx_dtr = n;
}

/* set transmit XON/XOFF flag */
void set_tx_xon(int n)
{
	tx_xon = n;
	tx_xonoff = FALSE;
}

/* set receive RTS/CTS flag */
void set_rx_rts(int n)
{
	rx_rts = n;
}

/* set receive DTR/DSR flag */
void set_rx_dtr(int n)
{
	rx_dtr = n;
}

/* set receive XON/XOFF flag */
void set_rx_xon(int n)
{
	rx_xon = n;
}

/* get transmit RTS flag */
int get_tx_rts(void)
{
	return tx_rts;
}

/* get transmit DTR/DSR flag */
int get_tx_dtr(void)
{
	return tx_dtr;
}

/* get transmit XON/XOFF flag */
int get_tx_xon(void)
{
	return tx_xon;
}

/* get receive RTS/CTS flag */
int get_rx_rts(void)
{
	return rx_rts;
}

/* get receive DTR/DSR flag */
int get_rx_dtr(void)
{
	return rx_dtr;
}

/* get receive XON/XOFF flag */
int get_rx_xon(void)
{
	return rx_xon;
}
