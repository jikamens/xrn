#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: utils.c,v 1.13 1995-01-25 03:17:52 jik Exp $";
#endif

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
 * utils.c: random utility functions for xrn
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <ctype.h>
#include <pwd.h>
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

#include "resources.h"

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
	if (isupper(*string)) {
	    *string = tolower(*string);
	}
    }
    return;
}

/* case insensitive, 24 character max, comparision for subjects */

int utSubjectCompare(str1, str2)
    register CONST char *str1, *str2;
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
	if (isupper(*str1)) {
	    c1 = tolower(*str1);
	} else {
	    c1 = *str1;
	}
	if (isupper(*str2)) {
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

#ifdef NEED_STRCASECMP

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef u_char
#define u_char  unsigned char
#endif

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static u_char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int strcasecmp(s1, s2)
    CONST char *s1, *s2;
{
	register u_char	*cm = charmap,
			*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while (cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return(0);
	return(cm[*us1] - cm[*--us2]);
}

int strncasecmp(s1, s2, n)
    CONST char *s1, *s2;
    size_t n;
{
	register u_char	*cm = charmap,
			*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while (--n >= 0 && cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return(0);
	return(n < 0 ? 0 : cm[*us1] - cm[*--us2]);
}

#endif /* NEED_STRCASECMP */

#ifdef REALLY_USE_LOCALTIME
static int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31};

/**********************************************************************
This function performs the function of mktime as declared in time.h,
but which has no definition.  It's the inverse of gmtime.
**********************************************************************/

static time_t makeTime _ARGUMENTS((struct tm *));

static time_t makeTime(tmp)
    struct tm *tmp;
{
  time_t ret;
  int i;

  if (tmp->tm_year < 70) return 0;
  ret = ((tmp->tm_year-70) / 4)*(366+365+365+365);
  switch ((tmp->tm_year-70) % 4) {
  case 1:
    ret += 365;
    break;
  case 2:
    ret += 365+365;
    if (tmp->tm_mon > 1) {
      ret += 1;
    }
    break;
  case 3:
    ret += 365+366+365;
  }
  for (i=0; i<tmp->tm_mon; i++) {
    ret += days[i];
  }
  ret += tmp->tm_mday-1;
  ret = ret*24+tmp->tm_hour;
  if (tmp->tm_isdst) {
    ret -= 1;
  }
  ret = ret*60+tmp->tm_min;
  ret = ret*60+tmp->tm_sec;
  return ret;
}

#ifdef ultrix
static char *getzonename _ARGUMENTS((int));
#endif

/**********************************************************************
This (ugly) function takes a source of the form "31 Aug 90 16:47:06 GMT"
and writes into dest the equivalent in local time.  If an invalid
source is given, the dest is a copy of the source.

Optionally, there may be a "XXX, " prepending the source where XXX is
a weekday name.
**********************************************************************/

int tconvert(dest, source)
    char *dest, *source;
{
  char *p, *fmt;
  int h, m, s, day, mon, year;
  struct tm *tmp, t;
  time_t then;
  int doWeekDay;
#if defined(apollo)
  int daylight;
#endif

  strcpy(dest, source);

  /* Parse date */
  p = source;
  if (!strncasecmp(p, "mon, ", 5) ||
      !strncasecmp(p, "tue, ", 5) ||
      !strncasecmp(p, "wed, ", 5) ||
      !strncasecmp(p, "thu, ", 5) ||
      !strncasecmp(p, "fri, ", 5) ||
      !strncasecmp(p, "sat, ", 5) ||
      !strncasecmp(p, "sun, ", 5)) {
    p += 5;
    doWeekDay = 1;
  } else {
/*    doWeekDay = 0; */
    doWeekDay = 1;		/* Let's put the weekday in all postings */
  }
  while (*p == ' ') {
    p++;
  }
  if (sscanf(p, "%d", &day) != 1) {
    return;
  }
  while (*p != ' ' && *p != '\0') {
    p++;
  }
  if (*p == '\0') {
    return;
  }
  while (*p == ' ' && *p != '\0') {
    p++;
  }
  if (*p == '\0') {
    return;
  }
  if (!strncasecmp(p, "jan", 3)) {
    mon = 0;
  } else if (!strncasecmp(p, "feb", 3)) {
    mon = 1;
  } else if (!strncasecmp(p, "mar", 3)) {
    mon = 2;
  } else if (!strncasecmp(p, "apr", 3)) {
    mon = 3;
  } else if (!strncasecmp(p, "may", 3)) {
    mon = 4;
  } else if (!strncasecmp(p, "jun", 3)) {
    mon = 5;
  } else if (!strncasecmp(p, "jul", 3)) {
    mon = 6;
  } else if (!strncasecmp(p, "aug", 3)) {
    mon = 7;
  } else if (!strncasecmp(p, "sep", 3)) {
    mon = 8;
  } else if (!strncasecmp(p, "oct", 3)) {
    mon = 9;
  } else if (!strncasecmp(p, "nov", 3)) {
    mon = 10;
  } else if (!strncasecmp(p, "dec", 3)) {
    mon = 11;
  } else {
    return;
  }
  while (*p != ' ' && *p != '\0') {
    p++;
  }
  if (*p == '\0') {
    return;
  }
  if (sscanf(p, "%d", &year) != 1) {
    return;
  }
  year = year % 100;

  /* Parse time */
  p = strrchr(source, ':');
  if (!p) {
    return;
  }
  p--;
  while (p > source && *p != ':') {
    p--;
  }
  while (p > source && *p != ' ') {
    p--;
  }
  if (!p) {
    return;
  }
  sscanf(p, "%d", &h);
  p = strchr(p, ':');
  if (!p++) {
    return;
  }
  sscanf(p, "%d", &m);
  p = strchr(p, ':');
  if (!p++) {
    return;
  }
  sscanf(p, "%d", &s);
  p = strchr(p, ' ');
  if (!p++) {
    return;
  }

  /* Confirm GMT */
  if (strcmp(p, "GMT")) {
    return;
  }

  t.tm_sec = s;
  t.tm_min = m;
  t.tm_hour = h;
  t.tm_mday = day;
  t.tm_mon = mon;
  t.tm_year = year;
  t.tm_isdst = 0;
  then = makeTime(&t);
  tmp = localtime(&then);
  
/* ascftime is non-standard, sigh.
  ascftime(dest, "%e %b %y %H:%M:%S %Z", tmp);
*/
  fmt = asctime(tmp);
  /* Make this look like the original format */
  p = dest;
  if (doWeekDay) {
    switch (tmp->tm_wday) {
    case 0:
      strcpy(dest, "Sun, ");
      break;
    case 1:
      strcpy(dest, "Mon, ");
      break;
    case 2:
      strcpy(dest, "Tue, ");
      break;
    case 3:
      strcpy(dest, "Wed, ");
      break;
    case 4:
      strcpy(dest, "Thu, ");
      break;
    case 5:
      strcpy(dest, "Fri, ");
      break;
    case 6:
      strcpy(dest, "Sat, ");
      break;
    }
    p += 5;
  }
  if (*(fmt+8) == ' ') {
    strncpy(p, fmt+9, 2);
    p += 2;
  } else {
    strncpy(p, fmt+8, 3);
    p += 3;
  }
  strncpy(p, fmt+4, 4);
  p += 4;
  if ((tmp->tm_year % 100) < 10) {
    sprintf(p, "0%d", tmp->tm_year % 100);
  } else {
    sprintf(p, "%d", tmp->tm_year % 100);
  }
  strcat(dest, fmt+10);
  p = strrchr(dest, ' ');

#if defined(sun) && !defined(SOLARIS)
  (void) strcpy(p+1, tmp->tm_zone);
#endif
  
#ifdef ultrix
  (void) strcpy(p+1, getzonename(tmp->tm_isdst));
#endif

#if defined(apollo)
  daylight = tmp->tm_isdst;
#endif

#if defined(apllo) || defined(SOLARIS)
  if (daylight) {
    strcpy(p+1, tzname[1]);
  } else {
    strcpy(p+1, tzname[0]);
  }
#endif

  if (*dest == ' ') {
    p = dest;
    while (*p != '\0') {
      *p = *(p+1);
      p++;
    }
  }
}

#ifdef ultrix

extern char *timezone();

static char * getzonename(isdst)
    int isdst;
{
	static char *name[2];
	struct timezone tz;

	if (isdst)
		isdst = 1;

	if (name[isdst] != NULL)
		return name[isdst];

	gettimeofday(NULL, &tz);
	name[isdst] = timezone(tz.tz_minuteswest, isdst);
	name[isdst] = XtNewString(name[isdst]);
	return name[isdst];
}

#endif /* ultrix */

#endif /* REALLY_USE_LOCALTIME */

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

    
