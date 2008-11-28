
#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/xthelper.c,v 1.5 1994-10-16 15:57:25 jik Exp $";
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
 * xthelper.c: routines for simplifying the use of the X toolkit
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "xmisc.h"
#include "xrn.h"
#include "xthelper.h"


void xthCenterWidget(widget, x, y)
    Widget widget;
    int x, y;
{
    Arg cargs[2];
    Dimension width, height;

    XtSetArg(cargs[0], XtNwidth, (XtPointer) &width);
    XtSetArg(cargs[1], XtNheight, (XtPointer) &height);
    XtGetValues(widget, cargs, XtNumber(cargs));

    x -= (int) width / 2;
    y -= (int) height / 2;
    if (x + (int) width > WidthOfScreen(XtScreen(widget))) {
	x = WidthOfScreen(XtScreen(widget)) - (int) width;
    }
    if (y + (int) height > HeightOfScreen(XtScreen(widget))) {
	y = HeightOfScreen(XtScreen(widget)) - (int) height;
    }

    if (x < 0) {
	x = 0;
    }
    if (y < 0) {
	y = 0;
    }
    XtSetArg(cargs[0], XtNx, x);
    XtSetArg(cargs[1], XtNy, y);
    XtSetValues(widget, cargs, XtNumber(cargs));
    return;
}

/*
 * center a window over the cursor
 *
 *   returns: void
 *
 */
void xthCenterWidgetOverCursor(widget)
    Widget widget;
{
    Window root, child;
    int x, y, dummy;
    unsigned int mask;

    (void) XQueryPointer(XtDisplay(widget), XtWindow(widget),
			 &root, &child,
			 &x, &y, &dummy, &dummy,
			 &mask);

    xthCenterWidget(widget, x, y);
    return;
}

void xthHandleAllPendingEvents()
{
    XEvent ev;
    XtAppContext app = XtWidgetToApplicationContext(TopLevel);

    XSync(XtDisplay(TopLevel), False);
    while (XtAppPending(app)) {
	XtAppNextEvent(app, &ev);
	XtDispatchEvent(&ev);
    }
    return;
}

void xthHandlePendingExposeEvents()
{
    XEvent ev;
    XtAppContext app = XtWidgetToApplicationContext(TopLevel);
    
    XSync(XtDisplay(TopLevel), False);
    while (XtAppPending(app) && XtAppPeekEvent(app, &ev) &&
	   (ev.type == Expose)) {
	XtAppNextEvent(app, &ev);
	XtDispatchEvent(&ev);
    }
    return;
}
