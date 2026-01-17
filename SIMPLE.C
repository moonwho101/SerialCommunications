/********************************************************************
* simple.c - Simple Comm 1.0 - A Simple Communications Program      *
*            Copyright (c) 1992 By Mark D. Goodwin                  *
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
#include "serial.h"

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

void main(void)
{
	int c;

	read_config();                      /* read in the config file */
	ansi_dsr = put_serial;              /* set ANSI routine */
	ansi_dsr_flag = TRUE;
	ansiout(12);
	ansiprintf("Simple Comm v1.0\n");
	ansiprintf("Copyright (c) 1992 By Mark D. Goodwin\n\n");
	ansiprintf("Initializing Modem.....\n\n");
	open_port(config.port, 4096);		/* open the comm port */
	/* configure the port if no carrier */
	if (carrier())
		ansiprintf("Already connected at %ld baud!\n\n", get_baud());
	else {
		set_port(config.baud_rate, config.parity ? 8 : 7,
			config.parity ? NO_PARITY : EVEN_PARITY, 1);
	}
	set_tx_rts(TRUE);					/* set XON/XOFF and RTS/CTS */
	set_tx_xon(TRUE);					/* flow control */
	set_rx_rts(TRUE);
	set_rx_xon(TRUE);
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
			c = getch();                /* get the key */
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
			put_serial((unsigned char)c); /* send the key out the port */
		}
		if (in_ready()) {				/* get and display incoming */
			c = get_serial();			/*  characters */
			ansiout(c);
		}
	}
}

/* file transfer error handler */
int e_handler(int c, long p, char *s)
{
	int k;

	switch (c) {
		case SENDING:            		/* file is being sent */
			ansiprintf("Sending: %s\n", strupr(s));
			ansiprintf("Length : %ld\n", p);
			break;
		case RECEIVING:                 /* file is being received */
			ansiprintf("Receiving: %s\n", strupr(s));
			ansiprintf("Length : %ld\n", p);
			break;
		case SENT:						/* block was sent */
			ansiprintf("%ld bytes sent.\r", p);
			break;
		case RECEIVED:					/* block was received */
			ansiprintf("%ld bytes received.\r", p);
			break;
		case ERROR:						/* an error occurred */
			ansiprintf("\nError: %s in block %ld\n", s, p);
			break;
		case COMPLETE:					/* file transfer complete */
			ansiprintf("\n%s\n", s);
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
	ansiprintf("\n");						/* display the menu */
	ansiprintf("<X> - Xmodem\n");
	ansiprintf("<K> - Xmodem-1K\n");
	ansiprintf("<Y> - Ymodem\n");
	ansiprintf("<G> - Ymodem-G\n");
	ansiprintf("<Q> - Quit\n");
	ansiprintf("\nPlease make a selection: ");
	while (TRUE) {
		while (!kbhit()) ;
		switch (toupper(getch())) {
			case 0:						/* handle extended keys */
				getch();
				break;
			case 'X':					/* Xmodem or Xmodem-1K */
			case 'K':
				/* prompt the user for the filename */
				ansiprintf("X\n\nPlease enter the filename: ");
				gets(line);
				ansiprintf("\n");
				if (!*line)
					return;
				recv_file(XMODEM, e_handler, line);	/* get the file */
				return;
			case 'Y':
				ansiprintf("Y\n\n");
				recv_file(YMODEM, e_handler, cdir);	/* get the file */
				return;
			case 'G':
				ansiprintf("G\n\n");
				recv_file(YMODEMG, e_handler, cdir); /* get the file */
				return;
			case 'Q':					/* quit */
				ansiprintf("Q\n\n");
				return;
		}
	}
}

/* upload a file */
void upload(void)
{
	int c;
	char line[81];

	ansiprintf("\n");						/* display the menu */
	ansiprintf("<X> - Xmodem\n");
	ansiprintf("<K> - Xmodem-1K\n");
	ansiprintf("<Y> - Ymodem\n");
	ansiprintf("<G> - Ymodem-G\n");
	ansiprintf("<Q> - Quit\n");
	ansiprintf("\nPlease make a selection: ");
	while (TRUE) {
		while (!kbhit()) ;
		switch (toupper(getch())) {
			case 0:						/* handle extended keys */
				getch();
				break;
			case 'X':
				/* prompt the user for the filename */
				ansiprintf("X\n\nPlease enter the filename: ");
				gets(line);
				ansiprintf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				xmit_file(XMODEM, e_handler, files); /* send the file */
				return;
			case 'K':
				/* prompt the user for the filename */
				ansiprintf("K\n\nPlease enter the filename: ");
				gets(line);
				ansiprintf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				xmit_file(XMODEM1K, e_handler, files); /* send the file */
				return;
			case 'Y':
				/* prompt the user for the filename */
				ansiprintf("Y\n\nPlease enter the filename: ");
				gets(line);
				ansiprintf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				xmit_file(YMODEM, e_handler, files); /* send the file */
				return;
			case 'G':
				/* prompt the user for the filename */
				ansiprintf("G\n\nPlease enter the filename: ");
				gets(line);
				ansiprintf("\n");
				if (!*line)
					return;
				files[0] = line;
				files[1] = NULL;
				xmit_file(YMODEMG, e_handler, files); /* send the file */
				return;
			case 'Q':
				ansiprintf("Q\n\n");	   	/* quit */
				return;
		}
	}
}

/* Help Routine */
void help(void)
{
	ansiprintf("\n");						/* display the special keys */
	ansiprintf("Alt-C - Configure Program\n");
	ansiprintf("Alt-D - Dial Phone\n");
	ansiprintf("Alt-H - Hang Up Modem\n");
	ansiprintf("Pg Up - Upload a File\n");
	ansiprintf("Pg Dn - Download a File\n");
	ansiprintf("Alt-X - Exit Program\n");
	ansiprintf("\n");
	ansiprintf("Press any key to continue.....\n");
	ansiprintf("\n");
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
	if (carrier()) {
		ansiprintf("\nStill connected.....Hang Up (Y/n)? ");
		while (TRUE) {
			if (kbhit()) {
				switch (toupper(getch())) {
					case 0:			 	/* handle extended keys */
						getch();
						continue;
					case 13:
					case 'Y':
						ansiprintf("Y\n\n"); /* hangup */
						hangup();
						break;
					case 'N':
						ansiprintf("N\n\n"); /* don't hangup */
						break;
					default:
						continue;
				}
				break;
			}
		}
	}
	close_port();						/* close the serial port */
	exit(0);							/* exit the program */
}

/* dial routine */
void dial(void)
{
	int br;
	char line[81], dstring[81], number[81];

	/* prompt user for the number to be dialed */
	ansiprintf("\nEnter the number you wish to dial: ");
	gets(number);
	if (!*number)
		return;
	sprintf(dstring, "%s%s", config.dial_prefix, number);
	while (TRUE) {
		ansiprintf("\nDailing %s\n", number);
		delay(2000);
		ansiprintf("Seconds Remaining: ");
		sprintf(line, "%s^M", dstring);
		modem_control_string(line);    	/* dial the number */
		get_modem_string(line, dstring); /* get a response */
		/* dial interrupted by user */
		if (!strcmp(line, "INTERRUPTED")) {
			ansiprintf("\n");
			return;
		}
		/* user wants to repeat the dial immediately */
		if (!strcmp(line, "RECYCLE")) {
			ansiprintf("\n");
			continue;
		}
		/* connection was made */
		if (strstr(line, "CONNECT") != NULL) {
			ansiprintf("\n%s\n", line);
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
		ansiprintf("\n%s\n", line);			/* display the result string */
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
	ansiprintf("%-2d", (i * 10) / 182);		/* display time remaining */
	while (TRUE) {
		/* check for user intervention */
		if (kbhit()) {
			switch (c = getch()) {
				case 0:					/* handle extended keys */
					getch();
					break;
				case 27:				/* ESC - abort dial */
					put_serial(13);
					sprintf(s, "INTERRUPTED");
					return s;
				case ' ':				/* SPACE - redial */
					put_serial(13);
					sprintf(s, "RECYCLE");
					return s;
			}
			continue;
		}
		/* get a character from the port if one's there */
		if (in_ready()) {
			switch (c = get_serial()) {
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
			ansiprintf("\b \b\b \b%-2d", (i * 10) / 182);
			if (!i) {
				*s = 0;
				put_serial(13);
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

	ansiprintf("\nHanging Up");
	last = mpeek(0, 0x46c);             /* get current system clock */
	i = 180;							/* try for about 10 seconds */
	set_dtr(FALSE);						/* drop DTR */
	while (carrier() && i) {			/* loop till loss of carrier */
		if (last != mpeek(0, 0x46c)) {  /*  or time out */
			i--;
			last = mpeek(0, 0x46c);
			ansiprintf(".");
		}
	}
	set_dtr(TRUE);						/* raise DTR */
	if (!carrier()) {					/* return if disconnect */
		ansiprintf("\n");
		purge_in();
		return;
	}
	modem_control_string(config.hangup_string); /* send software command */
	i = 180;							/* try for about 10 seconds */
	while (carrier() && i) {			/* loop till loss of carrier */
		if (last != mpeek(0, 0x46c)) {   /*  or time out */
			i--;
			last = mpeek(0, 0x46c);
			ansiprintf(".");
		}
	}
	purge_in();
	ansiprintf("\n");
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
							put_serial((unsigned char)(*s - 64));
						}
						break;
					default:
						put_serial(*s);	/* send the character */
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
		ansiprintf("\n");
		ansiprintf("<1> Serial Port     : %d\n", config.port);
		ansiprintf("<2> Baud Rate       : %ld\n", config.baud_rate);
		ansiprintf("<3> Parity          : %s\n",
			config.parity == P8N1 ? "8N1" : "7E1");
		ansiprintf("<4> Lock Serial Port: %s\n",
			config.lock_port ? "Yes" : "No");
		ansiprintf("<5> Init. String    : %s\n", config.initialization_string);
		ansiprintf("<6> Hangup String   : %s\n", config.hangup_string);
		ansiprintf("<7> Dial Prefix     : %s\n", config.dial_prefix);
		ansiprintf("<S> Save and Quit\n");
		ansiprintf("<Q> Quit\n");
		ansiprintf("\nPlease make a selection: ");
		while (TRUE) {
			while (!kbhit()) ;
			switch (toupper(getch())) {
				case 0:       			/* handle extended keys */
					getch();
					continue;
				case '1':				/* get new port */
					ansiprintf("1\n\n");
					ansiprintf("Please select the new serial port: ");
					gets(line);
					ansiprintf("\n");
					if (!(t = atoi(line)))
						break;
					/* make sure it's a valid port */
					if (t > 4 || !port_exist(t))
						ansiprintf("That port doesn't exist!\n");
					else {
						close_port();
						config.port = t;
						open_port(config.port, 4096);
					}
					break;
				case '2':				/* get new baud rate */
					ansiprintf("2\n\n");
					ansiprintf("Please enter the new baud rate: ");
					gets(line);
					ansiprintf("\n");
					if ((tl = atol(line))) {
						config.baud_rate = tl;
						set_baud(config.baud_rate);
					}
					break;
				case '3':				/* toggle data format */
					ansiprintf("3\n\n");
					config.parity = (config.parity == P8N1 ? P7E1 : P8N1);
					set_data_format(config.parity ? 8 : 7,
						config.parity ? NO_PARITY : EVEN_PARITY, 1);
					break;
				case '4':				/* toggle locked port flag */
					ansiprintf("4\n\n");
					config.lock_port = !config.lock_port;
					break;
				case '5':				/* get new init string */
					ansiprintf("5\n\n");
					ansiprintf("Please enter the new initialization string: ");
					gets(line);
					ansiprintf("\n");
					if (*line)
						sprintf(config.initialization_string, strupr(line));
					break;
				case '6':				/* get new hangup string */
					ansiprintf("6\n\n");
					ansiprintf("Please enter the new hangup string: ");
					gets(line);
					ansiprintf("\n");
					if (*line)
						sprintf(config.hangup_string, strupr(line));
					break;
				case '7':				/* get new dial prefix */
					ansiprintf("7\n\n");
					ansiprintf("Please enter the new dial prefix: ");
					gets(line);
					ansiprintf("\n");
					if (*line)
						sprintf(config.dial_prefix, strupr(line));
					break;
				case 'S':				/* save config and return */
					ansiprintf("S\n\n");
					write_config();
					return;
				case 'Q':           	/* quit */
					ansiprintf("Q\n\n");
					return;
				default:
					continue;
			}
			break;
		}
	}
}

