#ifndef CONFIG_H
#define CONFIG_H

/*
 * $Id: config.h,v 1.46 1994-12-12 16:14:19 jik Exp $
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
 * config.h: configurable defaults
 */

#ifdef OV_CAMBRIDGE
#define CONFIG_H_IS_OK
#endif

#ifndef CONFIG_H_IS_OK
#error "You must edit config.h appropriately for your site!"
#endif

/*
 * Place to report bugs and request new features.  This must be set to
 * something; you can either set it to an address pointing at the
 * local maintainer of XRN, or leave the default, which is the contact
 * address for the official XRN maintainer.
 */
#ifndef GRIPES
#define GRIPES "bug-xrn@cam.ov.com"
#endif

/* if you want xrn to provide a titlebar */
/* #define TITLEBAR */

/* display compilation time in the XRN title bar */
/* #define WANT_DATE */

/* if you want a short icon name */
#define SHORT_ICONNAME

/*
 * Note: You can probably skip ahead to the DISTRIBUTION section if you are
 * using InterNetNews - INN
 */

/*
 * Your organization name, to be placed in outgoing messages.  Optional.
 */
/* #define ORG_NAME "The Firm, Big Division" */

/*
 * Name of the organization is in this file.  Optional.  If the file
 * exists and is non-epty, it overrides ORG_NAME.
 */
#ifndef VMS
/* #define ORG_FILE "/usr/local/lib/news/organization" */
#else
/* #define ORG_FILE "SYS$LOGIN:ORG.NAME" */
#endif

/*
 * Name of the nntp server is in this file - can be overriden
 * by command line option, X resource, or environment variable.
 * Optional.
 */
#ifndef VMS
/* #define SERVER_FILE "/usr/local/news/server" */
#else
/* #define SERVER_FILE "SYS$LOGIN:NNTP.SERVER" */
#endif

/*
 * The following #define's control how the various host names that are
 * included in outgoing messages are determined.  For specific details
 * about how the host names are determined, including the order in
 * which things are searched and what different concepts of "host
 * name" XRN uses, see the comment above the getHeader() function in
 * compose.c.
 */

/*
 * Your internet domain name, to be used when posting or sending mail.
 *
 *  .Berkeley.EDU
 *  .mit.EDU
 *  .CSS.GOV
 *  .CS.NET
 *  .BITNET
 *  .UUCP
 *
 * If you're compiling for VMS, this must be defined for mail or
 * posting to work.  Otherwise, it's needed only when your host name
 * (which is determined in one of numerous ways) doesn't have a period
 * in it, and a domain isn't available from one of these other
 * sources: domainName X resource, DOMAIN environment variable, INN
 * "domain" configuration value (if using INN), DOMAIN_FILE (see
 * below), or DOMAIN_NAME.
 */
/* #define DOMAIN_NAME ".big.firm.com" */

/*
 * use the name server to find out your official (with domain) host name
 */
#ifndef RESOLVER
#define RESOLVER
#endif

/*
 * Define RETURN_HOST if you want a default return address host name.
 * Useful if your site uses MX records or something to address all
 * mail to a pseudo host (e.g. user@Berkeley.EDU)
 */
/* #define RETURN_HOST "big.firm.com" */

/*
 * The name of a file containing your site's UUCP name, if it's
 * different from your internet host name.  Optional.
 */
/* #define UUCPNAME  "/etc/uucpname" */

/* define this if you don't want xrn to add the hostname to the Path field */
/* #define HIDE_PATH */

/*
 * Host name to use in the Message-ID: (if GENERATE_EXTRA_FIELDS is
 * defined -- see below) and Path: fields.  You probably don't need to
 * define this.  See the comment above getHeader() in compose.c for
 * more information.  Optional.
 */
/* #define REAL_HOST "big.firm.com" */

/*
 * Host name to use in the Sender: field, if INEWS isn't defined (see
 * below).  Optional.
 */
/* #define SENDER_HOST "big.firm.com" */

#ifndef VMS
/*
 * Name of the host to use as the users host name in the From field
 * in a composition is in the file.  Optional.
 */
/* #define HIDDEN_FILE "/usr/local/news/hiddenhost" */

/*
 * Name of the host to use as the users hosts name in the Path field 
 * in a composition is in the file.  Ignored if INN is being used.
 * Optional.
 */
/* #define PATH_FILE "/usr/local/news/pathhost" */

/*
 * Name of the internet domain is in this file.  Optional.
 */
/* #define DOMAIN_FILE "/usr/local/news/domain" */

#else /* VMS */

/* #define HIDDEN_FILE "SYS$LOGIN:HIDDEN.HOST" */
/* #define PATH_FILE "SYS$LOGIN:PATH.HOST" */
/* #define DOMAIN_FILE "SYS$LOGIN:DOMAIN.NAME" */

#endif /* VMS */

/*
 * Default distribution.  If you do not set this, then an empty
 * Distribution: field will appear in postings being composed, and the
 * user can fill it in if he wants to.  It isn't a problem if he
 * doesn't, because most News server software strips empty fields.
 *
 * Note that setting this to "world" is *strongly* discouraged.  Many
 * sites do not support a "world" distribution.  If your site is
 * configured so that only articles with distribution "world" make it
 * to the outside world, your site is probably configured wrong, and
 * you should configure it so that postings with no distribution at
 * all make it to the outside world.  Otherwise, you're just asking
 * for many sites to drop postings from your site on the floor.
 * 
 * Some sites like postings to be local by default, to force people to
 * think before posting a message outside the local organization.
 * That's why "local" is the suggested value below.  However,  it's
 * commented out by default because I personally feel that the Usenet
 * is a larger community than just the local organization, and when
 * someone posts, they intend for their posting to go to that whole
 * community more often than not.
 */
/* #define DISTRIBUTION "local" */

/*
 * If you want X-newsreader and X-mailer lines, indicating the version
 * of xrn you are using, to be included in outgoing postings and mail
 * messages, define this.
 */
/* #define IDENTIFY_VERSION_IN_MESSAGES */

/*
 * maximum size of a signature file (in bytes), if file is bigger than this,
 * it is not included
 * 
 * The default is 4*80 (i.e., allow a four-line signature) plus a little slop.
 */
#define MAX_SIGNATURE_SIZE 330

/*
 * support newsgroup specific signature files that exist
 * in the same directory as the KILL files
 */
/* #define SIGNATURE_A_LA_LOCAL_KILL */

/*
 * Prefix for each line included from an article
 */
#define INCLUDEPREFIX	"|> "

/* prefetch articles and groups */
#define XRN_PREFETCH

#ifdef XRN_PREFETCH
/*
 * if there are more than this many unread articles in the next
 * group, do not prefetch it - must be a string, e.g., "100".
 * If it's "0", then there is no prefetching limit.
 */
#define XRN_PREFETCH_MAX "0"
/*
 * prefetch only the unread articles, rather than all articles
 * from first unread to last in the group
 */
#define XRN_PREFETCH_UNREAD_ONLY
#endif /* XRN_PREFETCH */

/*
 * number of article headers to prefetch when searching subjects
 * in the backwards direction
 */
#define SUBJECTS	10


/*
 * For debugging only.  Define if you want core dumps, rather than a
 * death notification box and an attempt at updating the .newsrc and
 * cleaning up the temporary files, by default.  Note that even if you
 * don't define this, you can get XRN to core dump on fatal errors by
 * running with "+dumpCore".
 */
/* #define DUMPCORE */

/*
 * use less memory at the expense of some cpu time
 *   I have had some reports that this may not be working... so use
 *   this at your own risk
 */
/* #define STUPIDMMU */

/*
 * for various bugs in the toolkit / widget set
 */
#define TEXT_WIDGET_WORKS_CORRECTLY

/*
 * Define this if you don't want the translations of the buttons in
 * the top button box to be the same as the translations for the rest
 * of the xrn window.  This is relevant mostly for Motif.  For
 * example, defining this will cause space to activate the button your
 * mouse is over, instead of "doing the right thing" (e.g., entering
 * the current newsgroup, paging the current article, going to the
 * next unread article, etc.), when you are using the Motif version of
 * xrn.
 *
 * Note that right now, translations are only forced in the top button
 * box.  I.e., hitting space when your mouse is over a button in the
 * bottom button box will activate that button when you are using
 * Motif whether or not this is defined.
 */
/* #define DONT_FORCE_BUTTON_TRANSLATIONS */

/* 
 * define TRANSLATIONS_NOT_FREED if you are using a pre-patchlevel 18 version
 * of X11 Release 4 
 *
 *  If you see a message about "NULL translation table", you need to undefine
 *  this.
 */
/* #define TRANSLATIONS_NOT_FREED */

/* These defines are probably no longer needed */
/* #define TRANSLATIONBUG */	/* DEC (VMS) X windows release		*/
/* #define ERRORBUG */		/* DEC (VMS) X windows release		*/
/* #define DECFOCUSPATCH */	/* for certain DEC window managers	*/

/*
 * If XFILESEARCHPATH is defined, then its contents are treated as a string 
 * which is appended to the end of the XFILESEARCHPATH environment variable.
 * The purpose of this is that if you are planning on installing xrn
 * (and its app-defaults file) in a non-standard location, you can make 
 * sure that it will find its app-defaults file by compiling the program 
 * with the path to the file set in XFILESEARCHPATH (for example, when 
 * I compile the program I #define XFILESEARCHPATH to be 
 * "/usr/sipb/lib/%T/%N", since I install it in the Student Information 
 * Processing Board (SIPB) filesystem.  This patch addresses a general 
 * flaw in X11R4's (and X11R3's) handling of application defaults file --
 * there is no way for the programmer to suggest to the toolkit
 * where to look for the file without mucking with environment variables.
 *
 * (Jonathan I. Kamens <jik@pit-manager.mit.edu>)
 */
/* #define XFILESEARCHPATH "path" */

/*
 * generate Message-ID and Date fields
 *
 *   NNTP POST should handle this, but does not for some
 *   implementations.  Do not define this if you are using INN.
 *
 * NOTE: This requires the strftime() C library function.  If you
 * don't have strftime(), you can use this.
 */
/* #define GENERATE_EXTRA_FIELDS */

/*
 * don't use the XHDR NNTP command for getting a single header field
 * from a single article (for performance reasons), use HEADER and 
 * cache information...
 */
/* #define DONT_USE_XHDR_FOR_A_SINGLE_ITEM */

/*
 * the active file of most currently used news systems has a problem:
 *   if the first article number equals the last article number for a
 *   newsgroup, this means that there are 0 OR 1 ARTICLES.
 *
 * Cnews, INN, and and Bnews 2.11 (patch #19) have fixed this problem.
 */
#define FIXED_ACTIVE_FILE

/*
 * deal with stupid C-news problem of NOT updating the low number
 * of the group entries in the active file - define this if you
 * have fixed the problem.  Not needed with INN.
 */
#define FIXED_C_NEWS_ACTIVE_FILE

/*
 * old versions of NNTP (pre-1.5.11) read the 'active' file only once
 * per session, so reissuing the 'LIST' command would always return
 * the EXACT same data.  New versions of NNTP will reread the file
 * for each LIST command.  If you are using 1.5.11 or greater, or INN
 * define NNTP_REREADS_ACTIVE_FILE
 */

#ifdef INN
#define NNTP_REREADS_ACTIVE_FILE
#endif /* INN */
/* #define NNTP_REREADS_ACTIVE_FILE */

/*
 * Do you want to use inews for postings?  For INN, it is suggested that you do.
 */
/* #define INEWS "/usr/local/bin/inews" */

/*
 * Does INEWS read the signature file? The version of INEWS for INN
 * reads the signature file by default.  If you define this incorrectly
 * you will probably end up with two signatures on postings.
 */
/* #define INEWS_READS_SIG */

/*
 * If you don't want XRN to eat type ahead, define this.
 *
 * This actually doesn't work right now.  I can't figure out how to
 * make it work easily.  If you can, feel free to submit a patch!
 */
/* #define DO_NOT_EAT_TYPE_AHEAD */

/*
 * display articles with local time rather than GMT
 *  if you are running SunOS 3.5, get rid of the '|| defined(sun)'
 *  define USE_LOCALTIME if you want to use this feature
 */

/* #define USE_LOCALTIME */

#if defined(USE_LOCALTIME) && (defined(SYSV) || defined(ultrix) || defined(apollo) ||  defined(sun))
#define REALLY_USE_LOCALTIME
#endif


#ifndef PRINTCOMMAND
#	ifdef VMS
#		define PRINTCOMMAND   "PRINT/DELETE"
#	else /* ! VMS */
#		ifdef SYSV
#			define PRINTCOMMAND "lp -sc"
#		else /* ! SYSV */
#			define PRINTCOMMAND "lpr"
#		endif /* SYSV */
#	endif /* VMS */
#endif /* ! PRINTCOMMAND */

#ifndef SENDMAIL
#ifndef VMS
#define SENDMAIL       "/usr/lib/sendmail -oi -t"
#else
#define SENDMAIL       "MAIL "
#endif
#endif

#define SAVEMODE       "normal,headers,onedir"
#define SAVE_DIR_DEFAULT	ONEDIR_SAVE
#ifndef VMS
#define SAVEDIR        "~/News"
#define NEWSRCFILE     "~/.newsrc"
#define SAVENEWSRCFILE "~/.oldnewsrc"
#define SIGNATUREFILE  "~/.signature"
#else
#define SAVEDIR	       "SYS$SCRATCH:"
#define NEWSRCFILE     "SYS$LOGIN:NEWS.RC"
#define SAVENEWSRCFILE "SYS$LOGIN:OLDNEWS.RC"
#define SIGNATUREFILE  "SYS$LOGIN:NEWS.SIGNATURE"
#endif
#define TOPLINES	"10"
#define MINLINES	"3"
#define MAXLINES	"8"
#define CANCELCOUNT	"40"
#define LINELENGTH	"0"
#define BREAKLENGTH	"0"
/* ONLYSHOW set to "0" turns off the feature */
#define ONLYSHOW	"0"
/* RESCAN_TIME is in seconds.  Default is 0 (never rescan automatically) */
#define RESCAN_TIME	"0"

#ifndef VMS
#define DEADLETTER     "~/dead.letter"
#define SAVEPOSTINGS   "~/Articles"
#define TEMPORARY_DIRECTORY "/tmp"
#else
#define DEADLETTER     "SYS$SCRATCH:"
#define SAVEPOSTINGS   "SYS$SCRATCH:"
#define TEMPORARY_DIRECTORY "SYS$SCRATCH:"
#endif

/* definitions to convert from e.g. ISO-646 to ISO-8859 */
#define XLATE
/* define the strings of equivalent characters */
#ifdef XLATE
/*	Swedish ISO-646 to ISO-8859-1 */
#   define XLATE_FROM	"[]\\{}|$@^`~"
#   define XLATE_TO	"\304\305\326\344\345\366\244\311\334\351\374"
#endif /* XLATE */


/*
 *
 * End of User/Site Configuration Parameters
 *
 */


/* SYSTEM V regex package */
#if defined(macII) || defined(aiws) || (defined(hpux) || defined(__hpux)) || (defined(SYSV) && !defined(i386) && !defined(_IBMR2)) || defined(SVR4) || defined(SCO)
#define SYSV_REGEX
#ifndef SYSV
#define SYSV
#endif
#endif

/* setenv vs. putenv */
#if defined(mips) || (defined(hpux) || defined(__hpux)) || defined(sun) || defined(i386) \
     || defined(_IBMR2)
#define USE_PUTENV
#endif

/* vfork supported */
#if defined(ultrix) || defined(sun) || defined(apollo) || (defined(sony) && !defined(SVR4)) || (defined(hpux) || defined(__hpux))
#define VFORK_SUPPORTED
#endif

/* define this if your popen uses vfork - ultrix uses fork... */
#if !defined(VFORK_SUPPORTED) || (!defined(ultrix) && (!defined(SYSV) || defined(sgi)))
#define POPEN_USES_INEXPENSIVE_FORK
#endif

/* bsd b* functions */
#if defined(sequent) || !defined(SYSV)
#define BSD_BFUNCS
#endif

/* v{s,f}printf functions */
#if defined(sequent) || defined(ibm032) || (defined(sony) && !defined(SYSV) && !defined(_ANSI_C_SOURCE))
#define NEED_VPRINTF
#endif

/*
 * define this if your system C library doesn't have the tempnam
 * function - note that there a number of buggy tempnam implementations
 * in various vendors libc's.  You might want to always define NEED_TEMPNAM
 * and not use the libc one at all.
 */
#if defined(convex) || (defined(sony) && !defined(SVR4)) || (!defined(sun) && !defined(ultrix) && !defined(SYSV) && !defined(_XOPEN_SOURCE)) || defined(ibm032) || defined(sequent) || defined(_IBMR2)
#define NEED_TEMPNAM
#else
#define utTempnam tempnam
#endif
     
/* strtok function */
/*
 * SunOS, at least 4.1.3 and above, has its own strtok and doesn't
 * need ours.  I'm not sure what the correct symbol to use for that
 * exact situation is; I've determined that on my SPARCstation LX
 * runnin 4.1.3, the compiler defines the "sparc", "sun" and "unix"
 * symbols, so I'm checking all of those three; it's possible that I'm
 * being too specific, or even that checking those three symbols isn't
 * being specific enough, but it's the best I can do.  If you know a
 * better way for me to check that I'm on a SunOS system that has
 * strtok, please let me know. - jik 10/23/94
 */
#if defined(sequent) || (!defined(SYSV) && !defined(_ANSI_C_SOURCE) && \
			 !(defined(sparc) && defined(sun) && defined(unix)))
#define NEED_STRTOK
#endif

/* strstr function */
/*
 * Although SunOS 4.1.3 has a strstr function in its C library, it's
 * incredibly inefficient and slows down the performance of XRN in
 * some cases noticeably.  Our version is faster, so we use it
 * instead.
 */
#if !defined(_ANSI_C_SOURCE) || \
    (defined(sparc) && defined(sun) && defined(unix) && !defined(SOLARIS))
#define NEED_STRSTR
#endif

#if defined(clipper) || (defined(sony) && defined(SVR4)) || (defined(i386) && defined(SYSV))
#define NEED_STRCASECMP
#endif

#if defined(mips) || (defined(hpux) || defined(__hpux)) || defined(sun) || (defined(i386) && !defined(sequent)) || defined(_IBMR2)
#define USE_PUTENV
#endif

/*
 * Define NO_FCHMOD if your C library doesn't have the fchmod() system
 * call.
 */
#if defined(SCO)
#define NO_FCHMOD
#endif

/*
 * Configuration that the XRN maintainer uses.  There's no point in
 * touching this.
 */

#ifdef OV_CAMBRIDGE
#define DOMAIN_NAME ".cam.ov.com"
#define RETURN_HOST "cam.ov.com"
#define SENDER_HOST "cam.ov.com"
#define ORG_NAME    "OpenVision Technologies, Inc."
#endif

#endif /* CONFIG_H */