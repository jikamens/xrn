#ifndef MESG_H
#define MESG_H

#include "butdefs.h"

/*
 * $Id: mesg.h,v 1.14 1996-05-02 12:29:00 jik Exp $
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

#define XRN_INFO 	(1<<0)
#define XRN_SERIOUS 	(1<<1)
#define XRN_APPEND	(1<<2)
#define XRN_SAME_LINE	(1<<3)
#define XRN_WARNING	(1<<4)

extern void _info _ARGUMENTS((char *, /* Boolean */ int));
#define INFO(msg) _info((msg), False)
#define infoNow(msg) _info((msg), True)

extern void mesgPane _VARARGUMENTS((int, int, char *, ...));
extern int newMesgPaneName _ARGUMENTS((void));

extern char error_buffer[2048];

BUTDECL(mesgDismiss);
BUTDECL(mesgClear);

#endif /* MESG_H */
