#ifndef XRN_H
#define XRN_H

#include "file_cache.h"

/*
 * $Id: xrn.h,v 1.30 1997-06-30 02:52:46 jik Exp $
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

#include <X11/Intrinsic.h>

/*
 * xrn.h: set up the main screens
 *
 */

/* global variables that represent the widgets that are dynamically changed */

extern Widget TopLevel;
extern XtAppContext TopContext;
extern Widget TopInfoLine;      /* top button info line                      */
extern Widget BottomInfoLine;   /* bottom button info line                   */

extern int XRNState;

extern int inCommand, inSubCommand;	/* executing a button function	     */

extern file_cache FileCache;

#define XRN_X_UP    0x01
#define XRN_NEWS_UP 0x10

#define LABEL_SIZE 128
#define HOST_NAME_SIZE 1024

#if XtSpecificationRelease < 6
extern XEvent *XtLastEventProcessed _ARGUMENTS((Display *));
extern void MyMainLoop _ARGUMENTS((XtAppContext));
extern Boolean MyDispatchEvent _ARGUMENTS((XEvent *));
#else
#define MyDispatchEvent(ev) XtDispatchEvent(ev)
#endif

#endif /* XRN_H */
