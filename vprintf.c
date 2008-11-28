
#include "config.h"
#include "utils.h"
#include <stdio.h>

#ifdef NEED_VPRINTF

/* Portable vfprintf  by Robert A. Larson <blarson@skat.usc.edu> */

/* Copyright 1989 Robert A. Larson.
 * Distribution in any form is allowed as long as the author
 * retains credit, changes are noted by their author and the
 * copyright message remains intact.  This program comes as-is
 * with no warentee of fitness for any purpouse.
 *
 * Thanks to Doug Gwen, Chris Torek, and others who helped clarify
 * the ansi printf specs.
 *
 * Please send any bug fixes and improvments to blarson@skat.usc.edu .
 * The use of goto is NOT a bug.
 */

/* Feb	7, 1989		blarson		First usenet release */

/* This code implements the vsprintf function, without relying on
 * the existance of _doprint or other system specific code.
 *
 * Define NOVOID if void * is not a supported type.
 *
 * Two compile options are available for efficency:
 *	INTSPRINTF	should be defined if sprintf is int and returns
 *			the number of chacters formated.
 *	LONGINT		should be defined if sizeof(long) == sizeof(int)
 *
 *	They only make the code smaller and faster, they need not be
 *	defined.
 *
 * UNSIGNEDSPECIAL should be defined if unsigned is treated differently
 * than int in argument passing.  If this is definded, and LONGINT is not,
 * the compiler must support the type unsingned long.
 *
 * Most quirks and bugs of the available sprintf fuction are duplicated,
 * however * in the width and precision fields will work correctly
 * even if sprintf does not support this, as will the n format.
 *
 * Bad format strings, or those with very long width and precision
 * fields (including expanded * fields) will cause undesired results.
 */

/* 
 * XRN changes (submitted by users) 2/1/91
 *
 *  - inclusion config.h, utils.h
 *  - addition of #ifdef NEED_VPRINTF / #endif (set by config.h)
 *  - changed #ifdef __STDC__ around varargs.h / stdargs.h to
 *     #ifdef XRN_USE_STDARG (defined in utils.h)
 *  - addition stdargs / varargs
 *  - combined vprintf.c and vsprintf.c into 1 file
 */

#ifdef OSK		/* os9/68k can take advantage of both */
#define LONGINT
#define INTSPRINTF
#endif

/*
 * The vax uses pcc, which doesn't understand void *, and has an
 * int-returning sprintf
 */
#if defined(vax)
#define NOVOID
#define INTSPRINTF
#endif

/*
 * The RT has an int-returning sprintf, and understands void * only if
 * hc is being used.
 */
#ifdef ibm032
#define INTSPRINTF
#  ifndef __HIGHC__
#  define NOVOID
#  endif
#endif

/*
 * The mips doesn't have an int-returning sprintf, and doesn't
 * understand void *.
 */
#ifdef mips
#define NOVOID
#endif
     
/* This must be a typedef not a #define! */
#define NOVOID
#ifdef NOVOID
typedef char *pointer;
#else
typedef void *pointer;
#endif

#ifdef	INTSPRINTF
#define Sprintf(string,format,arg)	(sprintf((string),(format),(arg)))
#else
#define Sprintf(string,format,arg)	(\
	sprintf((string),(format),(arg)),\
	strlen(string)\
)
#endif

#ifdef XRN_USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif

typedef int *intp;

int vsprintf(dest, format, args)
    char *dest;
    register char *format;
    va_list args;
{
    register char *dp = dest;
    register char c;
    register char *tp;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif

    tempfmt[0] = '%';
    while(c = *format++) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch(c = *format++) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			dp += Sprintf(dp, tempfmt, va_arg(args, long));
		    else
#endif
			dp += Sprintf(dp, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    dp += Sprintf(dp, tempfmt, va_arg(args, pointer));
		    break;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = dp - dest;
		    break;
		case '%':
		default:
		    *dp++ = c;
		    break;
	    }
	} else *dp++ = c;
    }
    *dp = '\0';
    return dp - dest;
}

int vfprintf(dest, format, args)
    FILE *dest;
    register char *format;
    va_list args;
{
    register char c;
    register char *tp;
    register int count = 0;
    char tempfmt[64];
#ifndef LONGINT
    int longflag;
#endif

    tempfmt[0] = '%';
    while(c = *format++) {
	if(c=='%') {
	    tp = &tempfmt[1];
#ifndef LONGINT
	    longflag = 0;
#endif
continue_format:
	    switch(c = *format++) {
		case 's':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, char *));
		    break;
		case 'u':
		case 'x':
		case 'o':
		case 'X':
#ifdef UNSIGNEDSPECIAL
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, unsigned long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, unsigned));
		    break;
#endif
		case 'd':
		case 'c':
		case 'i':
		    *tp++ = c;
		    *tp = '\0';
#ifndef LONGINT
		    if(longflag)
			count += fprintf(dest, tempfmt, va_arg(args, long));
		    else
#endif
			count += fprintf(dest, tempfmt, va_arg(args, int));
		    break;
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, double));
		    break;
		case 'p':
		    *tp++ = c;
		    *tp = '\0';
		    count += fprintf(dest, tempfmt, va_arg(args, pointer));
		    break;
		case '-':
		case '+':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ' ':
		case '#':
		case 'h':
		    *tp++ = c;
		    goto continue_format;
		case 'l':
#ifndef LONGINT
		    longflag = 1;
		    *tp++ = c;
#endif
		    goto continue_format;
		case '*':
		    tp += Sprintf(tp, "%d", va_arg(args, int));
		    goto continue_format;
		case 'n':
		    *va_arg(args, intp) = count;
		    break;
		case '%':
		default:
		    putc(c, dest);
		    count++;
		    break;
	    }
	} else {
	    putc(c, dest);
	    count++;
	}
    }
    return count;
}

int vprintf(format, args)
    char *format;
    va_list args;
{
    return vfprintf(stdout, format, args);
}

#endif
