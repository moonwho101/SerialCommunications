/********************************************************************
* protocol.c - Xmodem, Xmodem-1K, Ymodem, and Ymodem-G Protocols    *
*              Copyright (c) 1992 By Mark D. Goodwin                *
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __TURBOC__
#include <dir.h>
#else
#include <dos.h>
#endif
#include <stdarg.h>
#include <io.h>
#include "serial.h"

/* protocol constants */
#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CPMEOF 0x1A
#define TIMED_OUT 3
#define BAD_BLOCK 5
#define CRC_ERROR 7

/* calculate elapsed time */
#define elapsed ((clock() - start) / CLK_TCK)

/* buffer for data packets */
unsigned char buffer[1024];

/* CRC table */
unsigned short crctab[256] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

/* update CRC routine */
#define updcrc(d, s) (crctab[((s >> 8) & 0xff)] ^ (s << 8) ^ d)

/* get a character, but time out after a specified time limit */
static int timed_get(int n)
{
	clock_t start;

	start = clock();					/* get current time */
	while (TRUE) {
		if (in_ready())					/* char available? */
			return get_serial();		/* return it if one is */
		if (!carrier() || elapsed >= n) /* break out of loop if time out */
			break;
	}
	return -1;							/* return time out error */
}

/* build a data packet */
static int build_block(int l, FILE *file)
{
	int i, j;

	i = fread(buffer, 1, l, file);		/* read in the next block */
	if (!i)								/* return if EOF */
		return FALSE;
	for (j = i; j < l; j++)				/* save CPMEOF for rest of block */
		buffer[j] = CPMEOF;
	return TRUE;
}

/* build Ymodem header block */
static void header_block(char *fname, FILE *file)
{
	int i;
	long length;
	#ifdef __TURBOC__
	struct ftime ft;
	#else
	unsigned fd, ft;
	#endif
	struct tm t;

	for (i = 0; i < 128; i++)			/* zero out the buffer */
		buffer[i] = 0;
	if (fname != NULL) {
		sprintf(buffer, fname);			/* save the filename */
		fseek(file, 0L, SEEK_END);		/* get the length */
		length = ftell(file);
		fseek(file, 0L, SEEK_SET);
		#ifdef __TURBOC__
		getftime(fileno(file), &ft);  	/* figure the file mod date */
		t.tm_year = ft.ft_year + 80;
		t.tm_mon = ft.ft_month - 1;
		t.tm_mday = ft.ft_day;
		t.tm_hour = ft.ft_hour;
		t.tm_min = ft.ft_min;
		t.tm_sec = ft.ft_tsec * 2;
		t.tm_isdst = FALSE;
		#else
		_dos_getftime(fileno(file), &fd, &ft);
		t.tm_year = (fd >> 9) + 80;
		t.tm_mon = ((fd >> 5) & 0xf) - 1;
		t.tm_mday = (fd & 0x1f);
		t.tm_hour = (ft >> 11);
		t.tm_min = (ft >> 5) & 0x3f;
		t.tm_sec = (ft & 0x1f) * 2;
		t.tm_isdst = FALSE;
		#endif
		putenv("TZ=GMT0GMT");		   	/* adjust for time zones */
		tzset();
		/* save the length and mod date */
		sprintf(buffer + strlen(buffer) + 1, "%ld %lo", length, mktime(&t));
	}
}

/* abort transfer routine */
static void abort_transfer(void)
{
	int i;

	for (i = 0; i < 8; i++) 			/* send 8 CANs */
		put_serial(CAN);
}

/* parse the filename */
static void parse(char *p, char *f)
{
	int i;
	char *s;

	if ((s = strrchr(p, ':')) != NULL)	/* bump past drive spec */
		p = s + 1;
	if ((s = strrchr(p, '\\')) != NULL)	/* bump past path spec */
		p = s + 1;
	*f = 0;								/* save the filename */
	for (i = 0; i < 12; i++) {
		if (!*p)
			break;
		*f = *p;
		f++;
		*f = 0;
		p++;
	}
}

/* file transmit routine */
int xmit_file(int xtype, int (*error_handler)(int c, long p, char *s), char *files[])
{
	int i, tries, block = 0, bsize, cancel, retrans, crc, batch, nfiles = 0;
	int obits, oparity, ostop, oxon;
	unsigned int sum;
	long length, bsent;
	clock_t start;
	char *fname;
	FILE *file;
	char path[13];

	switch (xtype) {					/* set block size and batch flag */
		case XMODEM:
			bsize = 128;
			batch = FALSE;
			break;
		case XMODEM1K:
			bsize = 1024;
			batch = FALSE;
			break;
		case YMODEM:
			bsize = 1024;
			batch = TRUE;
			break;
		case YMODEMG:
			bsize = 1024;
			batch = TRUE;
			break;
		default:
			return FALSE;
	}
	obits = get_bits();					/* save current settings */
	oparity = get_parity();
	ostop = get_stopbits();
	oxon = get_tx_xon();
	set_data_format(8, NO_PARITY, 1);	/* set for 8-N-1 */
	set_tx_xon(FALSE);
	while (TRUE) {
		fname = files[nfiles];			/* get the next filename */
		nfiles++;
		/* return if none and if not a batch transfer */
		if (fname == NULL && !batch) {
			set_data_format(obits, oparity, ostop);
			set_tx_xon(oxon);
			return TRUE;
		}
		start = clock();				/* get clock */
		cancel = FALSE;					/* no CAN */
		while (TRUE) {
			/* abort if loss of carrier */
			if (!carrier()) {
				(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
			/* get a character from the receiver */
			if (in_ready()) {
				switch (get_serial()) {
					case NAK:			/* Use no CRCs */
						crc = FALSE;
						break;
					case 'C':      		/* Use CRCs */
						if (xtype == YMODEMG)
							xtype = YMODEM;
						crc = TRUE;
						break;
					case 'G':			/* Ymodem-G */
						if (xtype != YMODEMG)
							continue;
						crc = TRUE;
						break;
					case CAN:
						/* abort if second cancel */
						if (cancel) {
							(*error_handler)(ERROR, block, "Transfer Cancelled By Receiver");
							set_data_format(obits, oparity, ostop);
							set_tx_xon(oxon);
							return FALSE;
						}
						cancel = TRUE;
						continue;
					default:
						cancel = FALSE;
						continue;
				}
				break;
			}
			/* abort if time out */
			if (elapsed >= 60) {
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
		}
		if (fname != NULL) {
			/* open the file */
			if ((file = fopen(fname, "rb")) == NULL) {
				abort_transfer();
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
			/* parse filename */
			parse(fname, path);
			fseek(file, 0L, SEEK_END);
			length = ftell(file);
			fseek(file, 0L, SEEK_SET);
			/* send filename and length to the error handler */
			if (!(*error_handler)(SENDING, length, fname)) {
				abort_transfer();
				fclose(file);
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
			/* set number of bytes sent */
			bsent = 0L;
		}
		/* send header if Ymodem or Ymodem-G */
		if (batch) {
			/* build header block */
			header_block(fname, file);
			while (TRUE) {
				/* figure CRC */
				for (i = sum = 0; i < 128; i++)
					sum = updcrc(buffer[i], sum);
				/* send the block */
				put_serial(SOH);
				put_serial(0);
				put_serial(0xff);
				for (i = 0; i < 128; i++)
					put_serial(buffer[i]);
				sum = updcrc(0, sum);
				sum = updcrc(0, sum);
				put_serial((unsigned char)((sum >> 8) & 0xff));
				put_serial((unsigned char)(sum & 0xff));
				cancel = retrans = FALSE;
				start = clock();
				while (TRUE) {
					/* abort if loss of carrier */
					if (!carrier()) {
						(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
						if (fname != NULL)
							fclose(file);
						set_data_format(obits, oparity, ostop);
						set_tx_xon(oxon);
						return FALSE;
					}
					/* don't wait if Ymodem-G */
					if (xtype == YMODEMG)
						break;
					/* get ACK */
					if (in_ready()) {
						switch (get_serial()) {
							case NAK:	/* retransmit the block */
								cancel = FALSE;
								retrans = TRUE;
								break;
							case ACK:	/* block received ok */
								cancel = FALSE;
								break;
							case CAN:
								/* abort if second CAN */
								if (cancel) {
									if (fname != NULL)
										fclose(file);
									set_data_format(obits, oparity, ostop);
									set_tx_xon(oxon);
									return FALSE;
								}
								cancel = TRUE;
								continue;
							default:
								cancel = FALSE;
								continue;
						}
						break;
					}
					/* abort if time out */
					if (elapsed >= 60) {
						if (fname != NULL)
							fclose(file);
						set_data_format(obits, oparity, ostop);
						set_tx_xon(oxon);
						return FALSE;
					}
				}
				/* do it again if NAK */
				if (retrans)
					continue;
				/* exit if end of batch transmission */
				if (fname == NULL) {
					timed_get(10);
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					return TRUE;
				}
				break;
			}
			start = clock();			/* get clock */
			cancel = FALSE;				/* flag no CAN */
			while (TRUE) {
				/* abort if loss of carrier */
				if (!carrier()) {
					(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					fclose(file);
					return FALSE;
				}
				if (in_ready()) {
					switch (get_serial()) {
						case NAK:		/* no CRCs */
							crc = FALSE;
							break;
						case 'C':		/* use CRCs */
							if (xtype == YMODEMG)
								xtype = YMODEM;
							crc = TRUE;
							break;
						case 'G':		/* Ymodem-G */
							if (xtype != YMODEMG)
								continue;
							crc = TRUE;
							break;
						case CAN:
							/* abort if second CAN */
							if (cancel) {
								(*error_handler)(ERROR, block, "Transfer Cancelled By Receiver");
								set_data_format(obits, oparity, ostop);
								set_tx_xon(oxon);
								fclose(file);
								return FALSE;
							}
							cancel = TRUE;
							continue;
						default:
							cancel = FALSE;
							continue;
					}
					break;
				}
				/* abort if time out */
				if (elapsed >= 60) {
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					fclose(file);
					return FALSE;
				}
			}
		}
		block = 1;						/* starting block number */
		retrans = FALSE;
		while (TRUE) {
			/* build the new block if not rexmit */
			if (!retrans) {
				if (bsize == 1024 && length - ftell(file) < 1024L)
					bsize = 128;
				if (!build_block(bsize, file))
					break;
			}
			/* figure CRC or checksum */
			for (i = sum = 0; i < bsize; i++) {
				if (crc)
					sum = updcrc(buffer[i], sum);
				else
					sum += buffer[i];
			}
			/* send the block */
			if (bsize == 128)
				put_serial(SOH);
			else
				put_serial(STX);
			put_serial((unsigned char)(block & 0xff));
			put_serial((unsigned char)((255 - block) & 0xff));
			for (i = 0; i < bsize; i++)
				put_serial(buffer[i]);
			if (crc) {
				sum = updcrc(0, sum);
				sum = updcrc(0, sum);
				put_serial((unsigned char)((sum >> 8) & 0xff));
				put_serial((unsigned char)(sum & 0xff));
			}
			else
				put_serial((unsigned char)(sum & 0xff));
			cancel = retrans = FALSE;
			start = clock();
			while (TRUE) {
				/* abort if loss of carrier */
				if (!carrier()) {
					(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
					fclose(file);
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					return FALSE;
				}
				/* Ymodem-G */
				if (xtype == YMODEMG) {
					/* adjust no bytes sent */
					bsent += bsize;
					if (!(*error_handler)(SENT, bsent, "ACK")) {
						abort_transfer();
						fclose(file);
						set_data_format(obits, oparity, ostop);
						set_tx_xon(oxon);
						return FALSE;
					}
					/* abort if two CANs received */
					if (in_ready() && get_serial() == CAN &&
						in_ready() && get_serial() == CAN) {
						(*error_handler)(ERROR, block, "Transfer Cancelled By Receiver");
						fclose(file);
						set_data_format(obits, oparity, ostop);
						set_tx_xon(oxon);
						return FALSE;
					}
					break;
				}
				if (in_ready()) {
					switch (get_serial()) {
						case NAK:		/* retransmit */
							if (!(*error_handler)(ERROR, block, "NAK")) {
								abort_transfer();
								fclose(file);
 								set_data_format(obits, oparity, ostop);
								set_tx_xon(oxon);
								return FALSE;
							}
							cancel = FALSE;
							retrans = TRUE;
							break;
						case ACK:		/* block received ok */
							bsent += bsize;
							if (!(*error_handler)(SENT, bsent, "ACK")) {
								abort_transfer();
								fclose(file);
								set_data_format(obits, oparity, ostop);
								set_tx_xon(oxon);
								return FALSE;
							}
							cancel = FALSE;
							break;
						case CAN:
							/* abort if second CAN */
							if (cancel) {
								(*error_handler)(ERROR, block, "Transfer Cancelled By Receiver");
								fclose(file);
								set_data_format(obits, oparity, ostop);
								set_tx_xon(oxon);
								return FALSE;
							}
							cancel = TRUE;
							continue;
						default:
							cancel = FALSE;
							continue;
					}
					break;
				}
				/* abort if time out */
				if (elapsed >= 60) {
					fclose(file);
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					return FALSE;
				}
			}
			/* bump block number if not retransmit */
			if (retrans)
				continue;
			block = (block + 1) & 0xff;
		}
		/* close the file */
		fclose(file);
		start = clock();
		/* flag end of transfer */
		put_serial(EOT);
		while (TRUE) {
			/* abort if loss of carrier */
			if (!carrier()) {
				(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
			/* get ACK */
			if (in_ready()) {
				if (get_serial() != ACK) {
					put_serial(EOT);
					continue;
				}
				break;
			}
			/* abort if time out */
			if (elapsed >= 60) {
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
		}
		(*error_handler)(COMPLETE, block, "Transfer Successfully Completed");
		/* exit if not Ymodem or Ymodem-G */
		if (!batch) {
			set_data_format(obits, oparity, ostop);
			set_tx_xon(oxon);
			return TRUE;
		}
		/* adjust block size */
		bsize = 1024;
	}
}

/* get a block from transmitter */
static int getblock(int block, int crc)
{
	int c, l, i, sum;

	/* get a character */
	if ((c = timed_get(10)) == -1)
		return TIMED_OUT;
	switch (c) {
		case CAN:						/* two CANs? */
			if ((c = timed_get(1)) == CAN)
				return CAN;
			return TIMED_OUT;
		case SOH:						/* 128 byte block header */
			l = 128;
			break;
		case STX: 						/* 1024 byte block header */
			l = 1024;
			break;
		case EOT:      					/* end of transfer */
			return EOT;
		default:
			return TIMED_OUT;			/* return timed out */
	}
	if ((c = timed_get(1)) == -1)		/* get the block number */
		return TIMED_OUT;
	if (c != block)      				/* return if bad block number */
		return BAD_BLOCK;
	if ((c = timed_get(1)) == -1)		/* get complement */
		return TIMED_OUT;
	if (c != ((255 - block) & 0xff))	/* return if bad block number */
		return BAD_BLOCK;
	for (i = 0; i < l; i++)				/* get the block */
		if ((int)(buffer[i] = (unsigned char)timed_get(1)) == -1)
			return TIMED_OUT;
	/* get checksum */
	if (!crc) {
		if ((c = timed_get(1)) == -1)
			return TIMED_OUT;
		for (i = sum = 0; i < l; i++)
			sum += buffer[i];
	}
	/* get CRC */
	else {
		if ((sum = timed_get(1)) == -1)
			return TIMED_OUT;
		if ((c = timed_get(1)) == -1)
			return TIMED_OUT;
		c |= sum << 8;
		for (i = sum = 0; i < l; i++)
			sum = updcrc(buffer[i], sum);
		sum = updcrc(0, sum);
		sum = updcrc(0, sum);
	}
	/* check for checksum or CRC error */
	if (c != sum)
		return CRC_ERROR;
	/* return block size */
	if (l == 128)
		return SOH;
	return STX;
}

/* receive files routine */
int recv_file(int xtype, int(*error_handler)(int c, long p, char *s), char *path)
{
	int i, l, batch, block, crc;
	int obits, oparity, ostop, oxon;
	long length, received, moddate;
	char *fname, line[80];
	FILE *file;
	#ifdef __TURBOC__
	struct ftime ft;
	#else
	unsigned fd, ft;
	#endif
	struct tm *t;

	/* set batch flag */
	switch (xtype) {
		case XMODEM:
		case XMODEM1K:
			batch = FALSE;
			break;
		case YMODEM:
		case YMODEMG:
			batch = TRUE;
			break;
		default:
			return FALSE;
	}
	obits = get_bits();					/* save current settings */
	oparity = get_parity();
	ostop = get_stopbits();
	oxon = get_tx_xon();
	set_data_format(8, NO_PARITY, 1);	/* set to 8-N-1 */
	set_tx_xon(FALSE);
	while (TRUE) {
		crc = TRUE;						/* use CRCs */
		/* get transmitter going */
		if (xtype == YMODEMG)
			put_serial('G');
		else
			put_serial('C');
		block = batch ? 0 : 1;
		for (i = 0; i < 10; i++) {
			/* get a block */
			l = getblock(block, crc);
			/* abort if CAN or loss of carrier */
			if (l == CAN || !carrier()) {
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
			/* get transmitter going again */
			if (l != SOH && l != STX && l != EOT) {
				switch (xtype) {
					case XMODEM:
						if (i < 2)
							put_serial('C');	/* use CRCs */
						else {
							put_serial(NAK); 	/* no CRCs after two tries */
							crc = FALSE;
						}
						continue;
					case XMODEM1K:
					case YMODEM:
						put_serial('C');		/* use CRCs */
						continue;
					case YMODEMG:
						put_serial('G');		/* Ymodem-G */
						continue;
				}
			}
			break;
		}
		/* abort if transmitter doesn't respond after 10 tries */
		if (i == 10) {
			set_data_format(obits, oparity, ostop);
			set_tx_xon(oxon);
			return FALSE;
		}
		/* set filename, length, and number of bytes received */
		if (!batch) {
			fname = path;
			length = received = 0L;
		}
		else {
			if (!*buffer) {
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return TRUE;
			}
			sprintf(line, "%s\\%s", path, buffer);
			fname = line;
			if (!buffer[strlen(buffer) + 1]) {
				length = 0L;
				moddate = 0L;
			}
			else {
				sscanf(buffer + strlen(buffer) + 1, "%ld%lo", &length, &moddate);
				received = 0;
			}
		}
		/* open the file */
		if ((file = fopen(fname, "wb")) == NULL) {
			abort_transfer();
			set_data_format(obits, oparity, ostop);
			set_tx_xon(oxon);
			return FALSE;
		}
		/* send filename and length to the error handler */
		if (!(*error_handler)(RECEIVING, length, fname)) {
			abort_transfer();
			fclose(file);
			set_data_format(obits, oparity, ostop);
			set_tx_xon(oxon);
			return FALSE;
		}
		/* if batch signal get 1st block */
		if (batch) {
			crc = TRUE;
			put_serial(ACK);			/* ACK block 0 */
			/* tell tranmitter to send 1st block */
			if (xtype == YMODEMG)
				put_serial('G');
			else
				put_serial('C');
			block = 1;
			for (i = 0; i < 10; i++) {
				/* get a block */
				l = getblock(block, crc);
				/* abort if CAN or loss of carrier */
				if (l == CAN || !carrier()) {
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					fclose(file);
					return FALSE;
				}
				/* nudge transmitter again if no block */
				if (l != SOH && l != STX && l != EOT) {
					switch (xtype) {
						case YMODEM:
							put_serial('C');
							continue;
						case YMODEMG:
							put_serial('G');
							continue;
					}
				}
				break;
			}
			/* abort if failed after 10 attempts */
			if (i == 10) {
				abort_transfer();
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				fclose(file);
				return FALSE;
			}
		}
		/* get file */
		while (TRUE) {
			/* break out of loop if end of file */
			if (l == EOT) {
				(*error_handler)(COMPLETE, 0, "Transfer Successfully Completed");
				break;
			}
			/* write the block to disk */
			if (!batch || (batch && !length)) {
				fwrite(buffer, 1, l == SOH ? 128 : 1024, file);
				received += (l == SOH ? 128 : 1024);
			}
			else {
				if (length - received >= (l == SOH ? 128 : 1024)) {
					fwrite(buffer, 1, l == SOH ? 128 : 1024, file);
					received += (l == SOH ? 128 : 1024);
				}
				else {
					if (length - received) {
						fwrite(buffer, 1, (size_t)(length - received), file);
							received = length;
					}
				}
			}
			if (!(*error_handler)(RECEIVED, received, "Block Received")) {
				abort_transfer();
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				fclose(file);
				return FALSE;
			}
			/* increment block number */
			block = (block + 1) & 255;
			/* ACK if not Ymodem-G */
			if (xtype != YMODEMG)
				put_serial(ACK);
			for (i = 0; i < 10; i++) {
				/* get a block */
				l = getblock(block, crc);
				if (l == SOH || l == STX || l == EOT)
					break;
				/* abort if loss of carrier */
				if (!carrier()) {
					(*error_handler)(ERROR, block, "Loss of Carrier Detected!");
					set_data_format(obits, oparity, ostop);
					set_tx_xon(oxon);
					fclose(file);
					return FALSE;
				}
				switch (l) {
					case CAN:			/* abort if CAN */
						(*error_handler)(ERROR, block, "Cancelled by Sender");
						set_data_format(obits, oparity, ostop);
						set_tx_xon(oxon);
						fclose(file);
						return FALSE;
					case TIMED_OUT:		/* TIMED OUT */
						if (!(*error_handler)(ERROR, block, "Short Block")) {
							abort_transfer();
							set_data_format(obits, oparity, ostop);
							set_tx_xon(oxon);
							fclose(file);
							return FALSE;
						}
						break;
					case BAD_BLOCK:		/* bad block number */
						if (!(*error_handler)(ERROR, block, "Bad Block Number")) {
							abort_transfer();
							set_data_format(obits, oparity, ostop);
							set_tx_xon(oxon);
							fclose(file);
							return FALSE;
						}
						break;
					case CRC_ERROR:		/* CRC error */
						if (!(*error_handler)(ERROR, block, crc ? "CRC Error" : "Checksum Error") || xtype == YMODEMG) {
							abort_transfer();
							set_data_format(obits, oparity, ostop);
							set_tx_xon(oxon);
							fclose(file);
							return FALSE;
						}
						break;
				}
				/* send NAK if not Ymodem-G */
				if (xtype != YMODEMG)
					put_serial(NAK);
			}
			/* abort after 10 attempts */
			if (i == 10) {
				abort_transfer();
				set_data_format(obits, oparity, ostop);
				set_tx_xon(oxon);
				return FALSE;
			}
		}
		/* ACK EOT */
		put_serial(ACK);
		/* close the file */
		fclose(file);
		/* exit if not Ymodem or Ymodem-G */
		if (!batch)
			return TRUE;
		/* adjust file mod date */
		if (moddate && (file = fopen(fname, "rb")) != NULL) {
			putenv("TZ=GMT0GMT");
			tzset();
			t = localtime(&moddate);
			#ifdef __TURBOC__
			ft.ft_year = t->tm_year - 80;
			ft.ft_month = t->tm_mon + 1;
			ft.ft_day = t->tm_mday;
			ft.ft_hour = t->tm_hour;
			ft.ft_min = t->tm_min;
			ft.ft_tsec = t->tm_sec / 2;
			setftime(fileno(file), &ft);
			#else
			fd = (t->tm_year - 80) << 9;
			fd |= (t->tm_mon + 1) << 5;
			fd |= t->tm_mday;
			ft = t->tm_hour << 11;
			ft |= t->tm_min << 5;
			ft |= t->tm_sec / 2;
			_dos_setftime(fileno(file), fd, ft);
			#endif
			fclose(file);
		}
	}
}
