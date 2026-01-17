/********************************************************************
* ansi.c - ANSI Terminal Driver                                     *
*          Copyright (c) 1992 By Mark D. Goodwin                    *
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdarg.h>
#ifndef __TURBOC__
#include <graph.h>
#endif
#include "serial.h"

static int cnt, crow = 1, ccol = 1;
static unsigned char buffer[256];
int ansi_dsr_flag = FALSE;
int (*ansi_dsr)(unsigned char n);

/* check for numeric digit */
static int bindigit(int c)
{
	return c >= '0' && c <= '9';
}

/* convert ASCII to binary */
static unsigned char *ascbin(unsigned char *s, int *n)
{
	*n = 0;
	while (TRUE) {
		if (bindigit(*s)) {
			*n *= 10;
			*n += (*s - '0');
			s++;
			continue;
		}
		return s;
	}
}

/* convert IBM color attribute to ANSI text string */
char *ibmtoansi(int att, char *s)
{
	s[0] = 0;
	strcat(s, "\x1b[0");
	if (att & 0x80)
		strcat(s, ";5");
	if (att & 0x8)
		strcat(s, ";1");
	switch (att & 7) {
		case 0:
			strcat(s, ";30");
			break;
		case 1:
			strcat(s, ";34");
			break;
		case 2:
			strcat(s, ";32");
			break;
		case 3:
			strcat(s, ";36");
			break;
		case 4:
			strcat(s, ";31");
			break;
		case 5:
			strcat(s, ";35");
			break;
		case 6:
			strcat(s, ";33");
			break;
		case 7:
			strcat(s, ";37");
	}
	switch ((att >> 4) & 7) {
		case 0:
			strcat(s, ";40");
			break;
		case 1:
			strcat(s, ";44");
			break;
		case 2:
			strcat(s, ";42");
			break;
		case 3:
			strcat(s, ";46");
			break;
		case 4:
			strcat(s, ";41");
			break;
		case 5:
			strcat(s, ";45");
			break;
		case 6:
			strcat(s, ";43");
			break;
		case 7:
			strcat(s, ";47");
	}
	strcat(s, "m");
	return s;
}

#ifndef __TURBOC__
void clreol(void)
{
	int i, r1, c1, r2, c2;
	struct rccoord cpos;

	_gettextwindow(&r1, &c1, &r2, &c2);
	cpos = _gettextposition();
	_settextwindow(cpos.row, cpos.col, cpos.row, c2);
	_clearscreen(_GWINDOW);
	_settextwindow(r1, c1, r2, c2);
	_settextposition(cpos.row, cpos.col);
}
#endif

/* ANSI emulation routine */
void ansiout(int c)
{
	int n, row, col;
	#ifndef __TURBOC__
	int r1, c1, r2, c2;
	struct rccoord cpos;
	#endif
	unsigned char *bufptr;
	#ifdef __TURBOC__
	struct text_info tinfo;
	#endif

	if (!cnt) {
		if (c != 27)
			switch (c) {
				case 0:
					break;
				case '\n':
					#ifdef __TURBOC__
					gettextinfo(&tinfo);
					if (wherey() + 1 > tinfo.winbottom) {
						movetext(1, 2, tinfo.winright,
							tinfo.winbottom, 1, 1);
						gotoxy(1, tinfo.winbottom);
						clreol();
					}
					else
						gotoxy(1, wherey() + 1);
					#else
					_gettextwindow(&r1, &c1, &r2, &c2);
					cpos = _gettextposition();
					if (cpos.row + 1 > r2) {
						_scrolltextwindow(1);
						_settextposition(r2, 1);
						clreol();
					}
					else
						_settextposition(cpos.row + 1, 1);
					#endif
					break;
				case 8:
				case 127:
					#ifdef __TURBOC__
					if (wherex() != 1) {
						putch(8);
						putch(' ');
						putch(8);
					}
					#else
					cpos = _gettextposition();
					if (cpos.col != 1) {
						_settextposition(cpos.row, cpos.col - 1);
						_outmem(" ", 1);
						_settextposition(cpos.row, cpos.col - 1);
					}
					#endif
					break;
				case 13:
					#ifdef __TURBOC__
					gotoxy(1, wherey());
					#else
					cpos = _gettextposition();
					_settextposition(cpos.row, 1);
					#endif
					break;
				case '\t':
					do {
					#ifdef __TURBOC__
						putch(' ');
					} while (wherex() % 8 != 1);
					#else
						_outmem(" ", 1);
						cpos = _gettextposition();
					} while (cpos.row % 8 != 1);
					#endif
					break;
				case 12:
					#ifdef __TURBOC__
					clrscr();
					#else
					_clearscreen(_GWINDOW);
					#endif
					break;
				default:
					#ifdef __TURBOC__
					putch(c);
					#else
					_outmem((char *)&c, 1);
					#endif
			}
		else {
			buffer[cnt] = (unsigned char)c;
			cnt++;
		}
		return;
	}
	if (cnt == 1) {
		if (c == '[') {
			buffer[cnt] = (unsigned char)c;
			cnt++;
		}
		else {
			#ifdef __TURBOC__
			putch(27);
			#else
			_outmem("\x1b", 1);
			#endif
			if (c != 27) {
				#ifdef __TURBOC__
				putch(c);
				#else
				_outmem((char *)&c, 1);
				#endif
				cnt = 0;
			}
		}
		return;
	}
	if (cnt == 2) {
		switch (c) {
			case 's':
				#ifdef __TURBOC__
				crow = wherey();
				ccol = wherex();
				#else
				cpos = _gettextposition();
				crow = cpos.row;
				ccol = cpos.col;
				#endif
				cnt = 0;
				return;
			case 'u':
				#ifdef __TURBOC__
				gotoxy(ccol, crow);
				#else
				_settextposition(crow, ccol);
				#endif
				cnt = 0;
				return;
			case 'K':
				clreol();
				cnt = 0;
				return;
			case 'H':
			case 'F':
				#ifdef __TURBOC__
				gotoxy(1, 1);
				#else
				_settextposition(1, 1);
				#endif
				cnt = 0;
				return;
			case 'A':
				#ifdef __TURBOC__
				gotoxy(wherex(), wherey() - 1);
				#else
				cpos = _gettextposition();
				_settextposition(cpos.row - 1, cpos.col);
				#endif
				cnt = 0;
				return;
			case 'B':
				#ifdef __TURBOC__
				gotoxy(wherex(), wherey() + 1);
				#else
				cpos = _gettextposition();
				_settextposition(cpos.row + 1, cpos.col);
				#endif
				cnt = 0;
				return;
			case 'C':
				#ifdef __TURBOC__
				gotoxy(wherex() + 1, wherey());
				#else
				cpos = _gettextposition();
				_settextposition(cpos.row, cpos.col + 1);
				#endif
				cnt = 0;
				return;
			case 'D':
				#ifdef __TURBOC__
				gotoxy(wherex() - 1, wherey());
				#else
				cpos = _gettextposition();
				_settextposition(cpos.row, cpos.col - 1);
				#endif
				cnt = 0;
				return;
			default:
				if (bindigit(c)) {
					buffer[cnt] = (unsigned char)c;
					cnt++;
					return;
				}
				cnt = 0;
				return;
		}
	}
	if (bindigit(c) || c == ';') {
		buffer[cnt] = (unsigned char)c;
		cnt++;
		if (cnt > 256)
			cnt = 0;
		return;
	}
	bufptr = buffer + 2;
	buffer[cnt] = (unsigned char)c;
	switch (c) {
		case 'H':
		case 'F':
		case 'h':
		case 'f':
			bufptr = ascbin(bufptr, &row);
			if (*bufptr != ';') {
				#ifdef __TURBOC__
				gotoxy(1, row);
				#else
				_settextposition(row, 1);
				#endif
				cnt = 0;
				return;
			}
			bufptr++;
			if (!bindigit(*bufptr)) {
				cnt = 0;
				return;
			}
			ascbin(bufptr, &col);
			#ifdef __TURBOC__
			gotoxy(col, row);
			#else
			_settextposition(row, col);
			#endif
			cnt = 0;
			return;
		case 'A':
			ascbin(bufptr, &n);
			#ifdef __TURBOC__
			gotoxy(wherex(), wherey() - n);
			#else
			cpos = _gettextposition();
			_settextposition(cpos.row - n, cpos.col);
			#endif
			cnt = 0;
			return;
		case 'B':
			ascbin(bufptr, &n);
			#ifdef __TURBOC__
			gotoxy(wherex(), wherey() + n);
			#else
			cpos = _gettextposition();
			_settextposition(cpos.row + n, cpos.col);
			#endif
			cnt = 0;
			return;
		case 'C':
			ascbin(bufptr, &n);
			#ifdef __TURBOC__
			gotoxy(wherex() + n, wherey());
			#else
			cpos = _gettextposition();
			_settextposition(cpos.row, cpos.col + n);
			#endif
			cnt = 0;
			return;
		case 'D':
			ascbin(bufptr, &n);
			#ifdef __TURBOC__
			gotoxy(wherex() - n, wherey());
			#else
			cpos = _gettextposition();
			_settextposition(cpos.row, cpos.col - n);
			#endif
			cnt = 0;
			return;
		case 'n':
			ascbin(bufptr, &n);
			if (n == 6 && ansi_dsr_flag)
				(*ansi_dsr)('\x1b');
				(*ansi_dsr)('[');
				(*ansi_dsr)('u');
			cnt = 0;
			return;
		case 'J':
			ascbin(bufptr, &n);
			if (n == 2)
				#ifdef __TURBOC__
				clrscr();
				#else
				_clearscreen(_GWINDOW);
				#endif
			cnt = 0;
			return;
		case 'm':
			while (TRUE) {
				#ifdef __TURBOC__
				gettextinfo(&tinfo);
				#endif
				bufptr = ascbin(bufptr, &n);
				switch (n) {
					case 0:
						#ifdef __TURBOC__
						textattr(7);
						#else
						_setbkcolor(0L);
						_settextcolor(7);
						#endif
						break;
					case 1:
						#ifdef __TURBOC__
						textattr(tinfo.attribute | 0x08);
						#else
						_settextcolor(_gettextcolor() | 0x08);
						#endif
						break;
					case 5:
						#ifdef __TURBOC__
						textattr(tinfo.attribute | 0x80);
						#else
						_settextcolor(_gettextcolor() | 0x10);
						#endif
						break;
					case 30:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8);
						#else
						_settextcolor(_gettextcolor() & 0xf8);
						#endif
						break;
					case 31:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 4);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 4);
						#endif
						break;
					case 32:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 2);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 2);
						#endif
						break;
					case 33:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 6);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 6);
						#endif
						break;
					case 34:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 1);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 1);
						#endif
						break;
					case 35:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 5);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 5);
						#endif
						break;
					case 36:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 3);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 3);
						#endif
						break;
					case 37:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0xf8 | 7);
						#else
						_settextcolor(_gettextcolor() & 0xf8 | 7);
						#endif
						break;
					case 40:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f);
						#else
						_setbkcolor(0L);
						#endif
						break;
					case 41:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x40);
						#else
						_setbkcolor(4L);
						#endif
						break;
					case 42:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x20);
						#else
						_setbkcolor(2L);
						#endif
						break;
					case 43:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x60);
						#else
						_setbkcolor(6L);
						#endif
						break;
					case 44:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x10);
						#else
						_setbkcolor(1L);
						#endif
						break;
					case 45:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x50);
						#else
						_setbkcolor(5L);
						#endif
						break;
					case 46:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x30);
						#else
						_setbkcolor(3L);
						#endif
						break;
					case 47:
						#ifdef __TURBOC__
						textattr(tinfo.attribute & 0x8f | 0x70);
						#else
						_setbkcolor(7L);
						#endif
						break;
				}
				if (*bufptr == ';') {
					bufptr++;
					continue;
				}
				cnt = 0;
				return;
			}
		default:
			cnt = 0;
			return;
	}
}

void ansistring(char *s)
{
	while (*s)
		ansiout(*s++);
}

int ansiprintf(char *f, ...)
{
	int l;
	va_list m;
	char *b, *s;

	if ((b = (char *)malloc(1024)) == NULL)
		return -1;
	va_start(m, f);
	l = vsprintf(b, f, m);
	s = b;
	while (*s) {
		ansiout(*s);
		s++;
	}
	va_end(m);
	free(b);
	return l;
}
