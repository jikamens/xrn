#ifndef UTILS_H
#define UTILS_H

/*
 * $Header: /d/src/cvsroot/xrn/utils.h,v 1.12 1994-10-13 23:32:36 jik Exp $
 */

/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1988-1993, Ellen M. Sentovich and Rick L. Spickelmier.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of California not
 * be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The University
 * of California makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * utils.h: random utility functions and macros for xrn
 */

#include <stdio.h>

/*
 * for XtMalloc, etc.
 */
#include <X11/Intrinsic.h>

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef _ARGUMENTS
#ifndef _NO_PROTO
#define _ARGUMENTS(arglist) arglist
#else
#define _ARGUMENTS(arglist) ()
#endif
#endif

#ifdef VOID_SIGNAL
#define SIG_RET_T void
#else
#define SIG_RET_T int
#endif

#if defined(_ANSI_C_SOURCE)
typedef SIG_RET_T	(*SIG_PF0) _ARGUMENTS((int));
#else /* ! _ANSI_C_SOURCE */
typedef SIG_RET_T	(*SIG_PF0) _ARGUMENTS((int, ...));
#endif /* _ANSI_C_SOURCE */

#undef SIG_RET_T

#if !defined(_POSIX_SOURCE)
#ifdef NEED_STRTOK
extern char *strtok _ARGUMENTS((char *, char const *));
#endif
extern char *getenv _ARGUMENTS((const char *));
#else
#ifndef NOSTDHDRS
#include <stdlib.h>
#endif /* NOSTDHDRS */
#endif /* !_POSIX_SOURCE */
#if !defined(_XOPEN_SOURCE)
extern char *mktemp _ARGUMENTS((char *));
#else
#define mktemp(string)    tmpnam(name)
#endif
#if !defined(_OSF_SOURCE)
extern char *index _ARGUMENTS((const char *, int));
extern char *rindex _ARGUMENTS((const char *, int));
#else
#include <string.h>
#endif

#ifdef macII
extern int strcmp();
#endif

/* allocation macros */
#define ALLOC(type)           (type *) XtMalloc((unsigned) sizeof(type))
#define ARRAYALLOC(type, sz)  (type *) XtMalloc((unsigned) (sizeof(type) * (sz)))
#define NIL(type)             (type *) 0
#define FREE(item)            XtFree((char *) item), item = 0
#ifdef VMS
extern int utGroupToVmsFilename _ARGUMENTS((char *filename, char *group));
#endif
#define STREQ(a,b)            (strcmp(a, b) == 0)
#define STREQN(a,b,n)         (strncmp(a, b, n) == 0)

extern char *utTrimSpaces _ARGUMENTS((char *));
extern char *utTildeExpand _ARGUMENTS((char *));
extern char *utNameExpand _ARGUMENTS((char *));
extern int utSubstring _ARGUMENTS((char *, char *));
extern void utDowncase _ARGUMENTS((char *));
extern void utUnXlate _ARGUMENTS((char *));
extern void utXlate _ARGUMENTS((char *));

#define utStrlen(s)	((s) ? strlen(s) : 0)

extern int utSubjectCompare _ARGUMENTS((const char *, const char *));

#ifdef NEED_TEMPNAM
extern char *utTempnam _ARGUMENTS((char *, char *));
#endif

#ifdef SYSV_REGEX
extern char *regcmp();
extern char *regex();
#else
extern char *re_comp();
#endif

#if defined(__STDC__) && (!defined(__alpha) || defined(__osf__))
extern FILE *popen _ARGUMENTS((const char *, const char *));
#else
extern FILE *popen _ARGUMENTS((char *, char *));
#endif
extern int pclose _ARGUMENTS((FILE *));

#ifdef NEED_STRCASECMP
extern int strcasecmp _ARGUMENTS((const char *, const char *));
extern int strncasecmp _ARGUMENTS((const char *, const char *, size_t));
#endif

extern int tconvert _ARGUMENTS((char *, char *));

#if defined(VMS) || defined(SYSV) || defined(SOLARIS)
#define index strchr
#define rindex strrchr
#endif

#ifdef __STDC__
#define CONST const
#else
#define CONST
#endif

#ifdef NEED_STRSTR
extern char *strstr _ARGUMENTS((char const *, char const *));
#endif

#endif /* UTILS_H */
