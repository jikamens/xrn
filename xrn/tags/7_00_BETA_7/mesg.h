#ifndef MESG_H
#define MESG_H

/*
 * $Header: /d/src/cvsroot/xrn/mesg.h,v 1.3 1994-10-17 15:28:05 jik Exp $
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
 * mesg.h: display info, warning, and error messages
 *
 */

#define XRN_INFO 	1<<0
#define XRN_SERIOUS 	1<<1
#define XRN_APPEND	1<<2

extern void info _ARGUMENTS((char *));
extern void infoNow _ARGUMENTS((char *));
#if defined(__STDC__) && !defined(NOSTDHDRS)
extern void mesgPane _ARGUMENTS((int, char *, ...));
#else
extern void mesgPane();	/* can't have a prototype here because we */
			/* can't know the type of "va_alist"      */
#endif
extern void mesgDisableRedisplay _ARGUMENTS((void));
extern void mesgEnableRedisplay _ARGUMENTS((void));
extern void mesgDisableAppend _ARGUMENTS((void));
extern void mesgEnableAppend _ARGUMENTS((void));

extern char error_buffer[2048];

#endif /* MESG_H */
