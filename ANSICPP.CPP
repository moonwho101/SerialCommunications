/********************************************************************
* ansicpp.cpp - ANSI C++ Routines                                   *
*               Copyright (c) 1992 By Mark D. Goodwin               *
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sercpp.h"

int ANSI::printf(char *f, ...)
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
