/********************************************************************
* serial.h - Header file for Serial Comm Prog in C and C++          *
*            Copyright (c) 1992 By Mark D. Goodwin                  *
********************************************************************/
#ifndef __SERIALH__
#define __SERIALH__

#ifndef __TURBOC__
#include <time.h>
#endif

/* boolean constants */
#define TRUE 1
#define FALSE 0

/* parity constants */
#define NO_PARITY 0						/* no parity constant */
#define EVEN_PARITY 1					/* even parity constant */
#define ODD_PARITY 2					/* odd parity constant */

/* file transfer constants */
#define XMODEM 1
#define XMODEM1K 2
#define YMODEM 3
#define YMODEMG 4

/* file error handler constants */
#define SENDING 1						/* sending file code */
#define RECEIVING 2						/* receiving file code */
#define SENT 3							/* block sent ok code */
#define RECEIVED 4						/* block received ok code */
#define ERROR 5							/* error code */
#define COMPLETE 6						/* completed transfer code */

/* global variables */
extern int sibuff, eibuff, ilen;
extern unsigned char far *inbuff;
extern int rx_flow, rx_rts, rx_dtr, rx_xon;
extern int tx_rts, tx_dtr, tx_xon, tx_xonoff;
extern int fon, foff;
extern int base;
extern int ansi_dsr_flag;
extern int (*ansi_dsr)(unsigned char n);

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
void ansiout(int c);
int ansiprintf(char *f, ...);
void ansistring(char *s);
int carrier(void);
void close_port(void);
#ifndef __TURBOC__
void delay(clock_t n);
#endif
void fifo(int n);
long get_baud(void);
int get_bits(void);
int get_parity(void);
int get_rx_dtr(void);
int get_rx_rts(void);
int get_rx_xon(void);
int get_serial(void);
int get_stopbits(void);
int get_tx_dtr(void);
int get_tx_rts(void);
int get_tx_xon(void);
#ifdef __TURBOC__
void interrupt handler(void);
#else
void _interrupt _far handler(void);
#endif
char *ibmtoansi(int att, char *s);
int in_ready(void);
unsigned mpeek(unsigned seg, unsigned off);
void open_port(int port, int inlen);
int port_exist(int port);
void purge_in(void);
int put_serial(unsigned char n);
int recv_file(int xtype, int(*error_handler)(int c, long p, char *s), char *path);
void set_baud(long baud);
void set_data_format(int bits, int parity, int stopbit);
void set_dtr(int n);
void set_port(long baud, int bits, int parity, int stopbit);
void set_rx_dtr(int n);
void set_rx_rts(int n);
void set_rx_xon(int n);
void set_tx_dtr(int n);
void set_tx_rts(int n);
void set_tx_xon(int n);
int xmit_file(int xtype, int (*error_handler)(int c, long p, char *s), char *files[]);
#ifdef __cplusplus
}
#endif

#endif
