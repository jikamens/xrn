
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id$";
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
#ifdef DEBUG
    char *type, intbuf[5];
#endif

    XSync(XtDisplay(TopLevel), False);
    while (XtAppPending(TopContext)) {
	XtAppNextEvent(TopContext, &ev);
#ifdef DEBUG
	switch (ev.type) {
	case Expose: type = "Expose"; break;
	case MapNotify: type = "MapNotify"; break;
	case ConfigureNotify: type = "ConfigureNotify"; break;
	case ReparentNotify: type = "ReparentNotify"; break;
	default:
	    (void) sprintf(intbuf, "%d", ev.type);
	    type = intbuf;
	}
	fprintf(stderr, "xthHandleAllPendingEvents: %s event\n", type);
#endif
	MyDispatchEvent(&ev);
    }
    return;
}

void xthHandlePendingExposeEvents()
{
    XEvent ev;
    
    XSync(XtDisplay(TopLevel), False);
    while (XtAppPending(TopContext) && XtAppPeekEvent(TopContext, &ev)) {
	switch (ev.type) {
	case KeyPress:
	case KeyRelease:
	case ButtonPress:
	case ButtonRelease:
	    return;
	default:
	    XtAppNextEvent(TopContext, &ev);
	    MyDispatchEvent(&ev);
	}
    }
    return;
}

static void map_handler(widget, closure, event, continue_to_dispatch)
    Widget widget;
    XtPointer closure;
    XEvent *event;
    Boolean *continue_to_dispatch;
{
    if (event->type == MapNotify)
	*(Boolean *)closure = True;
    return;
}

static void expose_handler(widget, closure, event, continue_to_dispatch)
    Widget widget;
    XtPointer closure;
    XEvent *event;
    Boolean *continue_to_dispatch;
{
    if (event->type == Expose)
	*(Boolean *)closure = True;
    return;
}

void xthWaitForMapped(
		      _ANSIDECL(Widget,		w),
		      _ANSIDECL(Boolean,	expose_too)
		      )
     _KNRDECL(Widget,	w)
     _KNRDECL(Boolean,	expose_too)
{
    Boolean mapped = False, exposed = False;
    Status ret;
    XWindowAttributes attributes;
    XEvent ev;
    XWMHints *hints;
#ifdef DEBUG
    char *type, intbuf[5];
#endif

    hints = XGetWMHints(XtDisplay(w), XtWindow(w));
    if (hints) {
      if (hints->initial_state != NormalState)
	mapped = True;
      XFree((void *) hints);
    }
    if (mapped && !expose_too)
	return;

    XtAddEventHandler(w, StructureNotifyMask, False, map_handler,
		      (XtPointer) &mapped);
    if (expose_too)
	XtAddEventHandler(w, ExposureMask, False, expose_handler,
			  (XtPointer) &exposed);
	
    if (! (ret = XGetWindowAttributes(XtDisplay(w), XtWindow(w), &attributes)))
	/* bail! */
	goto done;

    if ((attributes.map_state != IsUnmapped) && !expose_too)
	goto done;

    while ((! mapped) && (expose_too && !exposed)) {
	XtAppNextEvent(TopContext, &ev);
#ifdef DEBUG
	switch (ev.type) {
	case Expose: type = "Expose"; break;
	case MapNotify: type = "MapNotify"; break;
	case ConfigureNotify: type = "ConfigureNotify"; break;
	case ReparentNotify: type = "ReparentNotify"; break;
	default:
	    (void) sprintf(intbuf, "%d", ev.type);
	    type = intbuf;
	}
	fprintf(stderr, "xthWaitForMapped: %s event\n", type);
#endif
	MyDispatchEvent(&ev);
    }

  done:
    XtRemoveEventHandler(w, StructureNotifyMask, False, map_handler,
			 (XtPointer) &mapped);
    if (expose_too)
	XtRemoveEventHandler(w, ExposureMask, False, expose_handler,
			     (XtPointer) &exposed);
	
    return;
}
