#ifndef BUTTONS_H
#define BUTTONS_H

#include <X11/Intrinsic.h>

#include "utils.h"

/*
 * $Id: buttons.h,v 1.13 1997-01-12 03:41:22 jik Exp $
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
 * buttons.h: create and handle the buttons
 *
 */

extern void determineMode _ARGUMENTS((void));
extern Boolean watchingGroup _ARGUMENTS((char *));
extern void doTheRightThing _ARGUMENTS((Widget, XEvent *, String *,
					Cardinal *));
extern void createButtons _ARGUMENTS((void));
extern void setButtonSensitive _ARGUMENTS((Widget, char *, Boolean));

extern void confirmBox _ARGUMENTS((String, int, int, void (*) _ARGUMENTS((void))));
extern String anyIterator _ARGUMENTS((Widget, String, Boolean,
				      Boolean, Boolean, long *));
extern void setTopInfoLine _ARGUMENTS((char *));
extern void setBottomInfoLine _ARGUMENTS((char *));
extern void swapMode _ARGUMENTS((void));
extern void abortClear _ARGUMENTS((void));
extern int abortP _ARGUMENTS((void));
extern void abortSet _ARGUMENTS((void));
extern void abortClear _ARGUMENTS((void));

typedef struct buttonList {
    String name;
    XtCallbackRec *callbacks;
    char *message;
} ButtonList;

#define TOP	0
#define BOTTOM	1

extern void doButtons _ARGUMENTS((char *, Widget, ButtonList *, int *, int));

extern char *LastGroup;

#endif /* BUTTONS_H */
