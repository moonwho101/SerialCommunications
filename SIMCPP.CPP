/********************************************************************
* simcpp.cpp - Simple Comm 1.0 - A Simple Communications Program    *
*              Copyright (c) 1992 By Mark D. Goodwin                *
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <ctype.h>
#ifndef __TURBOC__
#include <graph.h>
#include <direct.h>
#else
#include <dir.h>
#endif
#include "sercpp.h"

/* data format constants */
#define P8N1 1
#define P7E1 0

/* configuration file record structure */
typedef struct {
	int port;
	long baud_rate;
	int parity;
	int lock_port;
	char initialization_string[81];
	char hangup_string[41];
	char dial_prefix[41];
} CONFIG;

/* function prototypes */
void config_program(void);
void dial(void);
void download(void);
void exit_program(void);
char *get_modem_string(char *s, char *d);
void help(void);
void hangup(void);
void modem_control_string(char *s);
void read_config(void);
void save_config(void);
void upload(void);

char *files[10];
CONFIG config;
XFERPORT *port;
ANSI ansi;

void main(void)
{
	int c;

	read_config();                      /* read in the config file */
	ansi.out(12);
	ansi.printf("Simple Comm v1.0\n");
	ansi.printf("Copyright (c) 1992 By Mark D. Goodwin\n\n");
	ansi.printf("Initializing Modem.....\n\n");
	port = new XFERPORT(config.port, 4096);
	/* configure the port if no carrier */
	if (carrier())
		ansi.printf("Already connected at %ld baud!\n\n", get_baud());
	else {
		port->set_port(config.baud_rate, config.parity ? 8 : 7,
			config.parity ? NO_PARITY : EVEN_PARITY, 1);
	}
	port->set_tx_rts(TRUE);					/* set XON/XOFF and RTS/CTS */
	port->set_tx_xon(TRUE);					/* flow control */
	port->set_rx_rts(TRUE);
	port->set_rx_xon(TRUE);
	/* reset the modem and send initialization string if no carrier */
	if (!carrier()) {
		modem_control_string("ATZ^M");
		delay(1500);
		modem_control_string(config.initialization_string);
		delay(500);
	}
	/* main program loop */
	while (TRUE) {
		if (kbhit()) {
			c = getch();   				/* get the key */
			if (!c) {
				switch(getch()) {
					case 32:			/* Alt-D - Dials */
						dial();
						break;
					case 35:			/* Alt-H - Hangs Up */
						hangup();
						break;
					case 45:           	/* Alt-X - Exits the Program */
						exit_program();
						break;
					case 46:			/* Alt-C - Configure Program */
						config_program();
						break;
					case 59:			/* F1 - Displays Help Screen */
						help();
						break;
					case 73:     		/* Pg Up - Upload a File */
						upload();
						break;
					case 81:			/* Pg Dn - Download a File */
						download();
						break;
				}
				continue;
			}
			port->put((unsigned char)c); /* send the key out the port */
		}
		if (in_ready()) {				/* get and display incoming */
			c = port->get();			/*  characters */
			ansi.out(c);
		}
	}
}

/* file transfer error handler */
int e_handler(int c, long p, char *s)
{
	int k;

	switch (c) {
		case SENDING:            		/* file is being sent */
			ansi.printf("Sending: %s\n", strupr(s));
			ansi.printf("Length : %ld\n", p);
			break;
		case RECEIVING:                 /* file is being received */
			ansi.printf("Receiving: %s\n", strupr(s));
			ansi.printf("Length : %ld\n", p);
			break;
		case SENT:						/* block was sent */
			ansi.printf("%ld bytes sent.\r", p);
			break;
		case RECEIVED:					/* block was received */
			ansi.printf("%ld bytes received.\r", p);
			break;
		case ERROR:						/* an error occurred */
			ansi.printf("\nError: %s in block %ld\n", s, p);
			break;
		case COMPLETE:					/* file transfer complete */
			ansi.printf("\n%s\n", s);
			break;
	}
	if (kbhit()) {						/* check for key pressed */
		if (!(k = getch())) {			/*  abort if ESC pressed */
			getch();
			return TRUE;
		}
		if (k == 27)
			return FALSE;
	}
	return TRUE;
}

/* download file routine */
void download(void)
{
	int c;
	char cdir[81], line[81];

	getcwd(cdir, 81);      				/* get the current directory */
	ansi.printf("\n");						/* display the menu */
	ansi.printf("<X> - Xmodem\n");
	ansi.printf("<K> - Xmodem-1K\n");
	ansi.printf("<Y> - Ymodem\n");
	ansi.printf("<G> - Ymodem-G\n");
	ansi.printf("<Q> - Quit\n");
	ansi.printf("\nPlease make a selection: ");
	while (TRUE) {
		while (!kbhit()) ;
		switch (toupper(getch())) {
			case 0:						/* handle extended keys */
				getch();
				break;
			case 'X':					/* Xmodem or Xmodem-1K */
			case 'K':
				/* prompt the user for the filename */
				ansi.printf("X\n\nPlease enter the filename: ");
				gets(line);
				ansi.printf("\n");
				if (!*line)
					return;
				port->receive(XMODEM, e_handler, line);	/* get the file */
				return;
			case 'Y':
				ansi.printf("Y\n\n");
				port->receive(YMODEM, e_handler, cdir);	/* get the file */
				return;
			case 'G':
				ansi.printf("G\n\n");
				port->receive(YMODEMG, e_handler, cdir); /* get the file */
				return;
			case 'Q':					/* quit */
				ansi.printf("Q\n\n");
				return;
		}
	}
}

/* upload a file */
void upload(void)
{
	int c;
	char line[81];

	ansi.printf("\n");						/* display the menu */
	ansi.printf("<X> - Xmodem\n");
	ansi.printf("<K> - Xmodem-1K\n");
	ansi.printf("<Y> - Ymodem\n");
	ansi.printf("<G> - Ymodem-G\n");
	ansi.printf("<Q> - Quit\n");
	ansi.printf("\nPlease make a selection: ");
	while (TRUE) {
		while (!kbhit()) ;
		switch (toupper(getch())) {
			case 0:						/* handle extended keys */
				getch();
				break;
			case 'X':
				/* prompt the user for the filename */
				ansi.printf("X\n\nPlease enter the filename: ");
				gets(line);
				ansi.printf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				port->transmit(XMODEM, e_handler, files); /* send the file */
				return;
			case 'K':
				/* prompt the user for the filename */
				ansi.printf("K\n\nPlease enter the filename: ");
				gets(line);
				ansi.printf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				port->transmit(XMODEM1K, e_handler, files); /* send the file */
				return;
			case 'Y':
				/* prompt the user for the filename */
				ansi.printf("Y\n\nPlease enter the filename: ");
				gets(line);
				ansi.printf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				port->transmit(YMODEM, e_handler, files); /* send the file */
				return;
			case 'G':
				/* prompt the user for the filename */
				ansi.printf("G\n\nPlease enter the filename: ");
				gets(line);
				ansi.printf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				port->transmit(YMODEMG, e_handler, files); /* send the file */
				return;
			case 'Q':
				ansi.printf("Q\n\n");	   	/* quit */
				return;
		}
	}
}

/* Help Routine */
void help(void)
{
	ansi.printf("\n");						/* display the special keys */
	ansi.printf("Alt-C - Configure Program\n");
	ansi.printf("Alt-D - Dial Phone\n");
	ansi.printf("Alt-H - Hang Up Modem\n");
	ansi.printf("Pg Up - Upload a File\n");
	ansi.printf("Pg Dn - Download a File\n");
	ansi.printf("Alt-X - Exit Program\n");
	ansi.printf("\n");
	ansi.printf("Press any key to continue.....\n");
	ansi.printf("\n");
	while (!kbhit()) {				   	/* wait for a key */
		if (!getch())
			getch();
		return;
	}
}

/* Exit Program Routine */
void exit_program(void)
{
	/* if carrier, ask whether to D/C or not */
	if (port->carrier()) {
		ansi.printf("\nStill connected.....Hang Up (Y/n)? ");
		while (TRUE) {
			if (kbhit()) {
				switch (toupper(getch())) {
					case 0:			 	/* handle extended keys */
						getch();
						continue;
					case 13:
					case 'Y':
						ansi.printf("Y\n\n"); /* hangup */
						hangup();
						break;
					case 'N':
						ansi.printf("N\n\n"); /* don't hangup */
						break;
					default:
						continue;
				}
				break;
			}
		}
	}
	delete port;						/* close the serial port */
	exit(0);							/* exit the program */
}

/* dial routine */
void dial(void)
{
	int br;
	char line[81], dstring[81], number[81];

	/* prompt user for the number to be dialed */
	ansi.printf("\nEnter the number you wish to dial: ");
	gets(number);
	if (!*number)
		return;
	sprintf(dstring, "%s%s", config.dial_prefix, number);
	while (TRUE) {
		ansi.printf("\nDailing %s\n", number);
		delay(2000);
		ansi.printf("Seconds Remaining: ");
		sprintf(line, "%s^M", dstring);
		modem_control_string(line);    	/* dial the number */
		get_modem_string(line, dstring); /* get a response */
		/* dial interrupted by user */
		if (!strcmp(line, "INTERRUPTED")) {
			ansi.printf("\n");
			return;
		}
		/* user wants to repeat the dial immediately */
		if (!strcmp(line, "RECYCLE")) {
			ansi.printf("\n");
			continue;
		}
		/* connection was made */
		if (strstr(line, "CONNECT") != NULL) {
			ansi.printf("\n%s\n", line);
			/* set DTE rate to match the connection rate if port */
			/*  isn't locked */
			if (!config.lock_port) {
				br = atoi(line + 7);
				if (br)
					set_baud(br);
				else
					set_baud(300);
			}
			return;
		}
		ansi.printf("\n%s\n", line);			/* display the result string */
	}
}

/* get response string */
char *get_modem_string(char *s, char *d)
{
	int i, c;
	unsigned last;

	last = mpeek(0, 0x46c);
	i = 1092;
	*s = 0;
	ansi.printf("%-2d", (i * 10) / 182);		/* display time remaining */
	while (TRUE) {
		/* check for user intervention */
		if (kbhit()) {
			switch (c = getch()) {
				case 0:					/* handle extended keys */
					getch();
					break;
				case 27:				/* ESC - abort dial */
					port->put(13);
					sprintf(s, "INTERRUPTED");
					return s;
				case ' ':				/* SPACE - redial */
					port->put(13);
					sprintf(s, "RECYCLE");
					return s;
			}
			continue;
		}
		/* get a character from the port if one's there */
		if (port->in_ready()) {
			switch (c = port->get()) {
				case 13:				/* CR - return the result string */
					if (*s)
						return s;
					continue;
				default:
					if (c != 10) {		/* add char to end of string */
						s[strlen(s) + 1] = 0;
						s[strlen(s)] = (char)c;
						/* ignore RINGING and the dial string */
						if (!strcmp(s, "RINGING") || !strcmp(s, d))
							*s = 0;
					}
			}
		}
		/* check for timeout and display time remaining */
		if (last != mpeek(0, 0x46c)) {
			last = mpeek(0, 0x46c);
			i--;
			ansi.printf("\b \b\b \b%-2d", (i * 10) / 182);
			if (!i) {
				*s = 0;
				port->put(13);
				return s;
			}
		}
	}
}

/* Hang Up Modem Routine */
void hangup(void)
{
	int i;
	unsigned last;

	ansi.printf("\nHanging Up");
	last = mpeek(0, 0x46c);             /* get current system clock */
	i = 180;							/* try for about 10 seconds */
	port->set_dtr(FALSE);						/* drop DTR */
	while (port->carrier() && i) {			/* loop till loss of carrier */
		if (last != mpeek(0, 0x46c)) {  /*  or time out */
			i--;
			last = mpeek(0, 0x46c);
			ansi.printf(".");
		}
	}
	port->set_dtr(TRUE);						/* raise DTR */
	if (!port->carrier()) {					/* return if disconnect */
		ansi.printf("\n");
		port->purge_in();
		return;
	}
	modem_control_string(config.hangup_string); /* send software command */
	i = 180;							/* try for about 10 seconds */
	while (port->carrier() && i) {			/* loop till loss of carrier */
		if (last != mpeek(0, 0x46c)) {   /*  or time out */
			i--;
			last = mpeek(0, 0x46c);
			ansi.printf(".");
		}
	}
	port->purge_in();
	ansi.printf("\n");
}

/* Send Control String to Modem */
void modem_control_string(char *s)
{
	while (*s) {						/* loop for the entire string */
		switch (*s) {
			case '~':					/* if ~, wait a half second */
				delay(500);
				break;
			default:
				switch (*s) {
					case '^':			/* if ^, it's a control code */
						if (s[1]) {		/* send the control code */
							s++;
							port->put((unsigned char)(*s - 64));
						}
						break;
					default:
						port->put(*s);	/* send the character */
				}
				delay(50);				/* wait 50 ms. for slow modems */
		}
		s++;						   	/* bump the string pointer */
	}
}

/* read program configuration file */
void read_config(void)
{
	FILE *f;

	/* open the file, create default if it doesn't exist */
	if ((f = fopen("SIMPLE.CFG", "rb")) == NULL) {
		if ((f = fopen("SIMPLE.CFG", "wb")) != NULL) {
			config.port = 2;
			config.baud_rate = 2400;
			config.parity = P8N1;
			config.lock_port = FALSE;
			sprintf(config.initialization_string, "ATS0=0V1X4^M");
			sprintf(config.hangup_string, "~~~+++~~~ATH0^M");
			sprintf(config.dial_prefix, "ATDT");
			fwrite(&config, sizeof(CONFIG), 1, f);
			return;
		}
		return;
	}
	fread(&config, sizeof(CONFIG), 1, f); /* read in config info */
}

/* write program configuration file */
void write_config(void)
{
	FILE *f;

	/* write the config info */
	if ((f = fopen("SIMPLE.CFG", "w+b")) != NULL) {
		fwrite(&config, sizeof(CONFIG), 1, f);
		fclose(f);
	}
}

/* program configuration routine */
void config_program(void)
{
	int t;
	long tl;
	char line[81];

	while (TRUE) {
		/* display the menu */
		ansi.printf("\n");
		ansi.printf("<1> Serial Port     : %d\n", config.port);
		ansi.printf("<2> Baud Rate       : %ld\n", config.baud_rate);
		ansi.printf("<3> Parity          : %s\n",
			config.parity == P8N1 ? "8N1" : "7E1");
		ansi.printf("<4> Lock Serial Port: %s\n",
			config.lock_port ? "Yes" : "No");
		ansi.printf("<5> Init. String    : %s\n", config.initialization_string);
		ansi.printf("<6> Hangup String   : %s\n", config.hangup_string);
		ansi.printf("<7> Dial Prefix     : %s\n", config.dial_prefix);
		ansi.printf("<S> Save and Quit\n");
		ansi.printf("<Q> Quit\n");
		ansi.printf("\nPlease make a selection: ");
		while (TRUE) {
			while (!kbhit()) ;
			switch (toupper(getch())) {
				case 0:       			/* handle extended keys */
					getch();
					continue;
				case '1':				/* get new port */
					ansi.printf("1\n\n");
					ansi.printf("Please select the new serial port: ");
					gets(line);
					ansi.printf("\n");
					if (!(t = atoi(line)))
						break;
					/* make sure it's a valid port */
					if (t > 4 || !port_exist(t))
						ansi.printf("That port doesn't exist!\n");
					else {
						delete port;
						config.port = t;
						port = new XFERPORT(config.port, 4096);
					}
					break;
				case '2':				/* get new baud rate */
					ansi.printf("2\n\n");
					ansi.printf("Please enter the new baud rate: ");
					gets(line);
					ansi.printf("\n");
					if ((tl = atol(line))) {
						config.baud_rate = tl;
						port->set_baud(config.baud_rate);
					}
					break;
				case '3':				/* toggle data format */
					ansi.printf("3\n\n");
					config.parity = (config.parity == P8N1 ? P7E1 : P8N1);
					port->set_data_format(config.parity ? 8 : 7,
						config.parity ? NO_PARITY : EVEN_PARITY, 1);
					break;
				case '4':				/* toggle locked port flag */
					ansi.printf("4\n\n");
					config.lock_port = !config.lock_port;
					break;
				case '5':				/* get new init string */
					ansi.printf("5\n\n");
					ansi.printf("Please enter the new initialization string: ");
					gets(line);
					ansi.printf("\n");
					if (*line)
						sprintf(config.initialization_string, strupr(line));
					break;
				case '6':				/* get new hangup string */
					ansi.printf("6\n\n");
					ansi.printf("Please enter the new hangup string: ");
					gets(line);
					ansi.printf("\n");
					if (*line)
						sprintf(config.hangup_string, strupr(line));
					break;
				case '7':				/* get new dial prefix */
					ansi.printf("7\n\n");
					ansi.printf("Please enter the new dial prefix: ");
					gets(line);
					ansi.printf("\n");
					if (*line)
						sprintf(config.dial_prefix, strupr(line));
					break;
				case 'S':				/* save config and return */
					ansi.printf("S\n\n");
					write_config();
					return;
				case 'Q':           	/* quit */
					ansi.printf("Q\n\n");
					return;
				default:
					continue;
			}
			break;
		}
	}
}

