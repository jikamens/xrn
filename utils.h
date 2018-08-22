#ifndef UTILS_H
#define UTILS_H

/*
 * $Id$
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

/*
  This is necessary so that when <unistd.h> is included, it will
  execute a pragma which will allow vfork to function correctly when
  compiled with the Sun compiler.
  */
#ifdef __sparc
#ifndef sparc
#define sparc
#endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#ifdef sun

extern int gethostname();

#ifndef SOLARIS

#include <memory.h>
#include <strings.h>

/* Stupid SunOS header files are missing buttloads of declarations */
extern int printf(), fprintf(), fputc(), fputs(), fread(), fwrite(), sscanf();
extern int fgetc();
extern int fclose(), vfprintf(), vsprintf(), puts(), fscanf(), _filbuf();
extern int _flsbuf(), rewind(), fseek();
extern int toupper(), tolower();
extern time_t time();
extern int getdtablesize();
extern int system();
extern int read(), close(), fchmod(), rename();
extern int putenv();
extern int strcasecmp(), strncasecmp();
extern void bzero(), bcopy();

#endif /* SOLARIS */
#endif /* sun */

#ifdef linux
extern char *tempnam();
#endif

#include <sys/param.h>
#ifndef MAXPATHLEN
#ifdef SCO
#include <limits.h>
#define MAXPATHLEN _POSIX_PATH_MAX
#else /* ! SCO */
#define MAXPATHLEN 512	/* XXX On POSIX systems, there's a call to get this */
#endif
#endif

/*
 * for XtMalloc, etc.
 */
#include <X11/Intrinsic.h>

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#if !defined(NeedFunctionPrototypes)
#if  defined(FUNCPROTO) && !defined(_NO_PROTO)
#define	NeedFunctionPrototypes	1
#else
#define	NeedFunctionPrototypes	0
#endif
#endif

#if !defined(NeedVarargsPrototypes)
/*
 * The number of symbols I'm checking here seems a bit excessive.  For
 * example, it probably isn't necessary to check both FUNCPROTO&2 and
 * __STDC__.  However, I don't think it hurts to be cautious.
 */
#if defined(FUNCPROTO) && (FUNCPROTO&2) && !defined(_NO_PROTO) && \
	!defined(NOSTDHDRS) && defined(__STDC__)
#define	NeedVarargsPrototypes	1
#else
#define	NeedVarargsPrototypes	0
#endif
#endif

#ifndef _ARGUMENTS
#if NeedFunctionPrototypes
#define _ARGUMENTS(arglist) arglist
#define _ANSIDECL(type,arg) type arg
#define _KNRDECL(type,arg)
#else
#define _ARGUMENTS(arglist) ()
#define _ANSIDECL(type,arg) arg
#define _KNRDECL(type,arg) type arg;
#endif
#endif

#ifndef _VARARGUMENTS
#if NeedVarargsPrototypes
#define _VARARGUMENTS(arglist) arglist
#define XRN_USE_STDARG
#else
#define _VARARGUMENTS(arglist) ()
#undef XRN_USE_STDARG
#endif
#endif

#ifdef VOID_SIGNAL
#define SIG_RET_T void
#else
#define SIG_RET_T int
#endif

#ifdef sgi
typedef SIG_RET_T (*SIG_PF0) _ARGUMENTS((void));
#else /* ! sgi */
# if defined(_ANSI_C_SOURCE) || defined(linux) || defined(__bsdi__) || defined(SOLARIS) || defined(__hpux)
typedef SIG_RET_T	(*SIG_PF0) _ARGUMENTS((int));
# else /* ! _ANSI_C_SOURCE */
typedef SIG_RET_T	(*SIG_PF0) _VARARGUMENTS((int, ...));
# endif /* _ANSI_C_SOURCE */
#endif /* sgi */

#undef SIG_RET_T

#ifndef HAVE_STRTOK
extern char *strtok _ARGUMENTS((char *, char const *));
#endif

#if !defined(_POSIX_C_SOURCE)
extern char *getenv _ARGUMENTS((const char *));
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#if !defined(_XOPEN_SOURCE)
extern char *mktemp _ARGUMENTS((char *));
#else
#define mktemp(string)    tmpnam(name)
#endif
#if !defined(_OSF_SOURCE)
extern char *index _ARGUMENTS((const char *, int));
extern char *rindex _ARGUMENTS((const char *, int));
#endif

#ifdef macII
extern int strcmp();
#endif

/* allocation macros */
#define ALLOC(type)           (type *) XtMalloc((unsigned) sizeof(type))
#define ARRAYALLOC(type, sz)  (type *) XtMalloc((unsigned) (sizeof(type) * (sz)))
#define NIL(type)             (type *) 0
#define FREE(item)            XtFree((char *) item), item = 0
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
extern void utTempnamFree _ARGUMENTS((char *));
#else
#ifdef TEMPFILE_DEBUG
static char *tempfile_debug_ret;
#define utTempnam(a,b) ((tempfile_debug_ret = tempnam(a,b)), fprintf(stderr, "tempnam: %s at %s:%d\n", tempfile_debug_ret, __FILE__, __LINE__), tempfile_debug_ret)
#else
#define utTempnam tempnam
#endif /* TEMPFILE_DEBUG */
#define utTempnamFree free
#endif

extern char *utTempFile _ARGUMENTS((char *));

#ifdef TEMPFILE_DEBUG
#define utTempFile(a) ((tempfile_debug_ret = utTempFile(a)), fprintf(stderr, "utTempFile: %s at %s:%d\n", tempfile_debug_ret, __FILE__, __LINE__), tempfile_debug_ret)
#endif

#ifdef SYSV_REGEX
extern char *regcmp();
extern char *regex();
#else
#ifdef POSIX_REGEX
#include <regex.h>
#else
extern char *re_comp();
extern int re_exec();
#endif
#endif

#ifdef POPEN_USES_INEXPENSIVE_FORK
#define xrn_popen popen
#define xrn_pclose pclose
/* It really shouldn't be necessary to declare pclose here, since it's
   supposed to be declared in <stdio.h>, but many systems don't
   declare it there, and we need to assign its function pointer to a
   variable, so we need a declaration.  Sigh. */
extern int pclose _ARGUMENTS((FILE *));
#else
extern FILE *xrn_popen _ARGUMENTS((const char *, const char *));
extern int xrn_pclose _ARGUMENTS((FILE *));
#endif /* POPEN_USES_INEXPENSIVE_FORK */

#ifdef NEED_STRCASECMP
extern int strcasecmp _ARGUMENTS((const char *, const char *));
extern int strncasecmp _ARGUMENTS((const char *, const char *, size_t));
#endif

extern void tconvert _ARGUMENTS((char *, char *));

#if defined(SYSV) || defined(SOLARIS)
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#endif

#ifdef NEED_STRSTR
extern char *strstr _ARGUMENTS((char const *, char const *));
#endif

extern void do_chmod _ARGUMENTS((FILE *, char *, int));

#if defined(__STDC__) && !defined(UNIXCPP)
#define CONCAT(a,b) a##b
#define STRINGIFY(a) #a
#else
#define CONCAT(a,b) a/**/b
#define STRINGIFY(a) "a"
#endif

char *nntpServer _ARGUMENTS((void));

#ifdef GCC_WALL
#define WALL(a) a
#else
#define WALL(a)
#endif

char *findServerFile _ARGUMENTS((char *, Boolean, Boolean *));

int utDigits _ARGUMENTS((long int));

#ifdef BSD_BFUNCS
#ifndef memset
#define memset(_Str_, _Chr_, _Len_) bzero(_Str_, _Len_)
#endif
#ifndef memcpy
#define memcpy(_To_, _From_, _Len_) bcopy(_From_, _To_, _Len_)
#endif
#endif

#if defined(BSD_BFUNCS) || defined(NO_MEMMOVE)
#ifndef memmove
#define memmove(_To_, _From_, _Len_) bcopy(_From_, _To_, _Len_)
#endif
#endif

#if SIZEOF_INT_P <= SIZEOF_INT
#define POINTER_NUM_TYPE int
#define POINTER_PRINTF_FORMAT "0x%08x"
#elif SIZEOF_INT_P <= SIZEOF_LONG
#define POINTER_NUM_TYPE long
#define POINTER_PRINTF_FORMAT "0x%08lx"
#else
#error I do not know how big pointers are, fix configure.ac and utils.h
#endif

#endif /* UTILS_H */
