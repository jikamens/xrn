#ifndef UTILS_H
#define UTILS_H

/*
 * $Id: utils.h,v 1.44 1996-06-09 18:49:39 jik Exp $
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

#if defined(hpux) || defined(__hpux)
#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE
#define _INCLUDE_POSIX2_SOURCE
#endif
#endif

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

#if defined(sun) || defined(__hpux) || defined(aiws) || \
	defined(_POSIX_SOURCE) || defined(SVR4) || defined(__uxp__)
#include <unistd.h>
#endif

#ifdef sun

#include <stdlib.h>

extern int gethostname();

#ifndef SOLARIS

#include <memory.h>
#include <strings.h>

/* Stupid SunOS header files are missing buttloads of declarations */
extern int printf(), fprintf(), fputc(), fputs(), fread(), fwrite(), sscanf();
extern int fclose(), vfprintf(), vsprintf(), puts(), fscanf(), _filbuf();
extern int _flsbuf(), rewind(), fseek();
extern int toupper(), tolower();
extern time_t time();
extern int getdtablesize();
extern int system();
extern int read(), close(), fchmod(), rename();
extern int putenv();

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

#ifndef _ARGUMENTS
#if defined(FUNCPROTO) && !defined(_NO_PROTO)
#define _ARGUMENTS(arglist) arglist
#else
#define _ARGUMENTS(arglist) ()
#endif
#endif

#ifndef _VARARGUMENTS
/*
 * The number of symbols I'm checking here seems a bit excessive.  For
 * example, it probably isn't necessary to check both FUNCPROTO&2 and
 * __STDC__.  However, I don't think it hurts to be cautious.
 */
#if defined(FUNCPROTO) && (FUNCPROTO&2) && !defined(_NO_PROTO) && \
	!defined(NOSTDHDRS) && defined(__STDC__)
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

#if defined(_ANSI_C_SOURCE) || defined(linux) || defined(__bsdi__) || defined(SOLARIS)
typedef SIG_RET_T	(*SIG_PF0) _ARGUMENTS((int));
#else /* ! _ANSI_C_SOURCE */
typedef SIG_RET_T	(*SIG_PF0) _VARARGUMENTS((int, ...));
#endif /* _ANSI_C_SOURCE */

#undef SIG_RET_T

#if defined(__STDC__) || defined(sgi)
#define CONST const
#else
#define CONST
#endif

#if !defined(_POSIX_SOURCE)
#ifdef NEED_STRTOK
extern char *strtok _ARGUMENTS((char *, char CONST *));
#endif
extern char *getenv _ARGUMENTS((CONST char *));
#else
#if !defined(NOSTDHDRS) && !defined(sun) /* included above on sun */
#include <stdlib.h>
#endif /* NOSTDHDRS */
#endif /* !_POSIX_SOURCE */
#if !defined(_XOPEN_SOURCE)
extern char *mktemp _ARGUMENTS((char *));
#else
#define mktemp(string)    tmpnam(name)
#endif
#if !defined(_OSF_SOURCE)
extern char *index _ARGUMENTS((CONST char *, int));
extern char *rindex _ARGUMENTS((CONST char *, int));
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

extern int utSubjectCompare _ARGUMENTS((CONST char *, CONST char *));

#ifdef NEED_TEMPNAM
extern char *utTempnam _ARGUMENTS((char *, char *));
#define utGetarticle getarticle
#else
#ifdef TEMPFILE_DEBUG
static char *tempfile_debug_ret, *getarticle_debug_ret;
#define utTempnam(a,b) ((tempfile_debug_ret = tempnam(a,b)), fprintf(stderr, "tempnam: %s at %s:%d\n", tempfile_debug_ret, __FILE__, __LINE__), tempfile_debug_ret)
#define utGetarticle(a,b,c,d,e,f) ((getarticle_debug_ret = getarticle(a,b,c,d,e,f)), fprintf(stderr, "getarticle: %s at %s:%d\n", getarticle_debug_ret, __FILE__, __LINE__), getarticle_debug_ret)
#else
#define utTempnam tempnam
#define utGetarticle getarticle
#endif /* TEMPFILE_DEBUG */
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
extern FILE *xrn_popen _ARGUMENTS((CONST char *, CONST char *));
extern int xrn_pclose _ARGUMENTS((FILE *));
#endif /* POPEN_USES_INEXPENSIVE_FORK */

#ifdef NEED_STRCASECMP
extern int strcasecmp _ARGUMENTS((CONST char *, CONST char *));
extern int strncasecmp _ARGUMENTS((CONST char *, CONST char *, size_t));
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
extern char *strstr _ARGUMENTS((char CONST *, char CONST *));
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

char *findServerFile _ARGUMENTS((char *, /* Boolean */ int));

#if defined(__osf__) || defined(_POSIX_SOURCE) || defined(SOLARIS)
typedef CONST void * qsort_arg_type;
#else
typedef void * qsort_arg_type;
#endif

int utDigits _ARGUMENTS((long int));

#endif /* UTILS_H */
