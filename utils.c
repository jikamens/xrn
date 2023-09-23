/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1988-1993, Ellen M. Sentovich and Rick L. Spickelmier.
 * Copyright (c) 1994-2023, Jonathan Kamens.
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
 * utils.c: random utility functions for xrn
 */

#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <pwd.h>
#include <assert.h>
#ifdef aiws
struct passwd *getpwuid();
struct passwd *getpwnam();
#endif /* aiws */
#if defined(apollo)
#include <sys/time.h>
#endif
#ifdef SOLARIS
#include <time.h>
#endif
#include "news.h"
#include "server.h"
#include "resources.h"

extern time_t get_date _ARGUMENTS((char *));

#define USER_NAME_SIZE 32

#ifdef SYSV_REGEX
/* 
 * kludge alert:  this is here because (on A/UX 1.1) linking with
 * the PW lib (necessary to get the regular expression routines, 
 * regcmp/regex), causes symbol 'Error' to be undefined.
 * 	glenn@mathcs.emory.edu 10/17/89 
 *
 * SYSV_REGEX may be overkill, need for macII and HPUX
 *    (Andy.Linton@comp.vuw.ac.nz 11/8/89)
 */
int Error; 
#endif
#include "xrn.h"

/*
 * trim leading and trailing spaces from a string (and newlines)
 *
 *   NOTE: this function modifiess the argument
 *
 *   returns: the modified string
 *
 */
char * utTrimSpaces(str)
    register char *str;
{
    char *end = &str[utStrlen(str) - 1];

    while ((*str == ' ') || (*str == '\n') || (*str == '\t')) {
	str++;
    }
    while ((*end == ' ') || (*end == '\n') || (*end == '\t')) {
	*end = '\0';
	end--;
    }
    return(str);
}

/*
 * tilde expand a file name
 *
 *   returns: the expanded name of the file (in a static buffer)
 *            or NIL(char) 
 */
char * utTildeExpand(filename)
    char *filename;    /* file name, possibly with a '~'             */
{
#ifdef aiws
    static char dummy[MAXPATH];
#else
    static char dummy[MAXPATHLEN];
#endif /* aiws */
    char username[USER_NAME_SIZE], *loc;
    struct passwd *pw;
    
    if ((filename == NIL(char)) || STREQ(filename, "")) {
	return(NIL(char));
    }

    if (filename[0] != '~') {
	(void) strcpy(dummy, filename);
	return(dummy);
    }

    /* tilde at the beginning now */
    if (filename[1] == '/') {
	/* current user */
	char *home;
	
	if ((home = getenv("HOME")) == NIL(char)) {
	    /* fall back on /etc/passwd */
	    if ((pw = getpwuid(getuid())) == NIL(struct passwd)) {
		return(NIL(char));
	    }
	    (void) sprintf(dummy, "%s%s", pw->pw_dir, &filename[1]);
	} else {
	    (void) sprintf(dummy, "%s%s", home, &filename[1]);
	}
	    
    } else {
	if ((loc = index(filename, '/')) == NIL(char)) {
	    /* error - bad filename */
	    return(NIL(char));
	}
	(void) strncpy(username, &filename[1], loc - &filename[1]);
	username[loc - &filename[1]] = '\0';
	if ((pw = getpwnam(username)) == NIL(struct passwd)) {
	    return(NIL(char));
	}
	(void) sprintf(dummy, "%s%s", pw->pw_dir, loc);
    }
    return(dummy);
}

char *
utNameExpand(filename)
char *filename;    /* file name, possibly with a '~'             */
/*
 * Use DOTDIR to expand a startup file name only if the filename is a
 * relative pathname. Otherwise call utTildeExpand()
 *
 *   returns: the expanded name of the file (in a static buffer)
 *            or NIL(char) 
 */
{
#ifdef aiws
    static char dummy2[MAXPATH];
#else
    static char dummy2[MAXPATHLEN];
#endif /* aiws */
    char *dotdir;
    
    if ((filename == NIL(char)) || STREQ(filename, "")) {
	return(NIL(char));
    }

    if (filename[0] != '~' && filename[0] != '/')
    {
	if ((dotdir = getenv("DOTDIR")) != NIL(char))
	    (void) sprintf(dummy2, "%s/%s", dotdir, filename);
	else
	    (void) strcpy(dummy2, filename);
	return(dummy2);
    }
    else {
	return(utTildeExpand(filename));
    }
}

int utSubstring(string, sub)
    char *string;
    char *sub;
{
    int i;
    int srcLen = utStrlen(string);
    int subLen = utStrlen(sub);

    if (srcLen < subLen)
	return(0);

    for (i = 0; i < srcLen - subLen + 1; i++) {
	if (STREQN(&string[i], sub, utStrlen(sub))) {
	    return(1);
	}
    }
    return(0);
}

/*
 * down case the characters in a string (in place)
 */
void utDowncase(string)
    register char *string;
{
    for ( ; *string != '\0'; string++) {
	if (isupper((unsigned char)*string)) {
	    *string = tolower(*string);
	}
    }
    return;
}

/* case insensitive, 24 character max, comparision for subjects */

int utSubjectCompare(str1, str2)
    register const char *str1, *str2;
{
    int count = 0;
    char c1, c2;

    while (count++ < 24) {
	if (!*str1 && !*str2) {
	    return 0;
	}
	if (!*str1) {
	    return -1;
	}
	if (!*str2) {
	    return 1;
	}
	if (isupper((unsigned char)*str1)) {
	    c1 = tolower(*str1);
	} else {
	    c1 = *str1;
	}
	if (isupper((unsigned char)*str2)) {
	    c2 = tolower(*str2);
	} else {
	    c2 = *str2;
	}
	if (c1 != c2) {
	    return (c1 - c2);
	}
	str1++;
	str2++;
    }
    return 0;
}

void tconvert(dest, source)
    char *dest, *source;
{
  time_t converted = get_date(source);
  char buf[30] /* ctime only takes 26, but who knows if it'll change? */;

  if (converted == (time_t)-1) {
    if (dest != source)
      strcpy(dest, source);
  }
  else {
    strcpy(buf, ctime(&converted));
    /* RFC 822 requires a comma after the day, but ctime doesn't
       provide one, so we'll have to insert it.  We check explicitly
       to make sure the date begins with three alphabetics and a
       space, in case the ctime format changes out from under us. */
    if (isalpha(buf[0]) && isalpha(buf[1]) && isalpha(buf[2]) &&
	isspace(buf[3])) {
      strncpy(dest, buf, 3);
      dest[3] = ',';
      strcpy(dest + 4, buf + 3);
    }
    else
      strcpy(dest, buf);
  }
}

#ifdef XLATE

#define UC(x)	(unsigned char)(x)
/* translate a character string in place */

static Boolean	inited = 0;
static char	xlate[ 256 ];
static char	unxlate[ 256 ];

static void
XlateInit()
{
    char	*to = XLATE_TO, *from = XLATE_FROM;
    int		i;

    for ( i = 256; --i >= 0; )
	xlate[ i ] = unxlate[ i ] = i;

    while (*to && *from) {
	unxlate[ UC(*to) ] = *from;
	xlate[ UC(*from++) ] = *to++;
    }
    inited = 1;
}

void
utXlate(s)
char *s;
{
    if (!inited) XlateInit();

    do {
	*s = xlate[ UC(*s) ];
    } while(*++s);
}

void
utUnXlate(s)
char *s;
{
    if (!inited) XlateInit();

    do {
	*s = unxlate[ UC(*s) ];
    } while(*++s);
}
#endif /* XLATE */

/*
  To ensure that a file's modes are correct, this function should be
  called twice for the file.  The first time should be when the file
  is still open, and fp should be the FILE * and name should be null.
  If fchmod is supported, it will be used to change the modes of the
  file.  The second time should be immediately after closing the file,
  anddd fp should be null and name should be the name of the file.  If
  fchmod *isn't* supported, then chmod will be used to change the
  modes of the file.

  If you want to do either the chmod or the fchmod right away, then
  you can pass in non-null fp and name, and one or the other of the
  two operations will be done.
  */
void do_chmod(fp, name, mode)
    FILE *fp;
    char *name;
    int mode;
{
#ifdef NO_FCHMOD
    if (name)
	(void) chmod(name, mode);
#else
    if (fp)
	(void) fchmod(fileno(fp), mode);
#endif
    return;
}

    
/*
  Return the correct NNTP server to use, or null if it can't be
  determined.

  Order in which things are checked:

  * -nntpServer command-line option
  * NNTPSERVER environment variable
  * nntpServer X resource
  * SERVER_FILE, if it's defined
  */
char *nntpServer()
{
    char *server;

    if (app_resources.cmdLineNntpServer)
	return(app_resources.cmdLineNntpServer);
    else if ((server = getenv("NNTPSERVER")))
	return(server);
    else if (app_resources.nntpServer)
	return(app_resources.nntpServer);
#ifdef SERVER_FILE
    else if ((server = getserverbyfile(SERVER_FILE)))
	return(server);
#endif /* SERVER_FILE */
    else
	return(0);
}

char *findServerFile(
		     _ANSIDECL(char *,		basename),
		     _ANSIDECL(Boolean,		prefer_long),
		     _ANSIDECL(Boolean *,	returned_long)
		     )
     _KNRDECL(char *,		basename)
     _KNRDECL(Boolean,		prefer_long)
     _KNRDECL(Boolean *,	returned_long)
{
  char *server_name = nntpServer();
  char *long_name = 0, *expanded;

  if (! (expanded = utNameExpand(basename)))
    return 0;

  if (server_name) {
    long_name = XtMalloc(strlen(expanded) + strlen(server_name) + 2);
    (void) sprintf(long_name, "%s-%s", expanded, server_name);
    if (! access(long_name, R_OK)) {
      if (returned_long)
	*returned_long = True;
      return(long_name);
    }
  }

  if ((! access(expanded, R_OK)) || (! prefer_long) || (! long_name)) {
    XtFree(long_name);
    if (returned_long)
      *returned_long = False;
    return(XtNewString(expanded));
  }

  if (returned_long)
    *returned_long = True;
  return(long_name);
}


/*
  Creates a temporary file in the same directory as the file passed
  into it, so that the temporary file can be rename()d to be the file
  name passed in, when it's done being created.

  This is different from tempnam() or utTempnam(), which might put
  the file in $TMPDIR instead of the specified directory.

  The file name returned is allocated and should be freed when it is
  no longer needed.
  */
char *utTempFile(orig_name)
char *orig_name;
{
  static char new_name[MAXPATHLEN];
  char *ptr;
  static int instance = 0;

  ptr = strrchr(orig_name, '/');

  if (ptr) {
    *ptr = '\0';
    assert(strlen(orig_name) < MAXPATHLEN-2);
    (void) sprintf(new_name, "%s/", orig_name);
    *ptr++ = '/';
  }
  else {
    (void) strcpy(new_name, "./");
    ptr = orig_name;
  }

  /* The 20 here is arbitrary; I'm assuming it's enough space to
     contain a period, an instance number, a hyphen, a PID, and
     another hyphen. */
  assert(strlen(new_name) + strlen(ptr) + 20 < MAXPATHLEN);

  (void) sprintf(&new_name[strlen(new_name)], ".%d-%d-%s", instance++,
		 getpid(), ptr);

  return XtNewString(new_name);
}

/*
 * Calculate the number of digits in an integer.  Sure, I could use a
 * logarithm function, but that would require relying on a sane math
 * library on all systems.  The technique used in this function is
 * gross, but what the heck, it works.
 */

int utDigits(num)
    long int num;
{
    char int_buf[20]; /* An article number longer than twenty digits?
			 I'll be dead by then! */

    (void) sprintf(int_buf, "%ld", num);
    return(strlen(int_buf));
}

