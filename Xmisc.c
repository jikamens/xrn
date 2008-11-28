
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
 * xmisc.c: routines for handling miscellaneous x functions
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Command.h>
#include "news.h"
#include "xthelper.h"
#include "resources.h"
#include "internals.h"
#include "xrn.h"
#include "xmisc.h"
#include "busyCursor.h"

/* XRN icon */

#if SUPPORT_SILLY_CALVIN_ICON
#include "calvin.icon"
#endif
#include "xrn.icon"

static Pixmap
getpm()
{
    unsigned int width, height;
    unsigned char *bits;

#if SUPPORT_SILLY_CALVIN_ICON
    if (app_resources.calvin) {
	width = calvin_width;
	height = calvin_height;
	bits = calvin_bits;
    } else {
#endif
	width = xrn_width;
	height = xrn_height;
	bits = xrn_bits;
#if SUPPORT_SILLY_CALVIN_ICON
    }
#endif

    return XCreateBitmapFromData(XtDisplay(TopLevel), XtScreen(TopLevel)->root,
				 (char *) bits, width, height);
}


void
xmSetIconAndName(it)
IconType it;
{
    static char       *PrevName = NULL, *OldName = NULL;
    static Pixmap     PrevPm = None, OldPm = None;
    char      *name WALL(= 0);
    Pixmap    pm WALL(= 0);
    Arg               arg;

    if (OldPm == None) {
	XtSetArg(arg, XtNiconPixmap, &OldPm);
  	XtGetValues(TopLevel, &arg, 1);
 	PrevPm = OldPm;
    }

    if (app_resources.iconPixmap == None) {
	app_resources.iconPixmap = getpm();
    }

    if (app_resources.unreadIconPixmap == None) {
	app_resources.unreadIconPixmap = app_resources.iconPixmap;
    }

    switch (it) {
	case InitIcon :
	    PrevPm = pm = app_resources.iconPixmap;
	    PrevName = name = OldName = app_resources.iconName;
	    break;
	case UnreadIcon :
	    PrevPm = pm = app_resources.unreadIconPixmap;
	    PrevName = name = app_resources.unreadIconName;
	    break;
	case ReadIcon :
	    PrevPm = pm = app_resources.iconPixmap;
	    PrevName = name = app_resources.iconName;
	    break;
	case BusyIcon :
	    if ((pm = app_resources.busyIconPixmap) == None)
		pm = OldPm;
	    if ((name = app_resources.busyIconName) == NULL)
 		name = OldName;
 	    break;
 	case PrevIcon :
 	    pm = PrevPm;
 	    name = PrevName;
 	    break;
    }

    if (OldPm != pm) {
 	XtSetArg(arg, XtNiconPixmap, pm);
  	XtSetValues(TopLevel, &arg, 1);
 	OldPm = pm;
    }

    if (name == NULL) {
 	name = app_resources.iconName;
    }

    if (OldName != name) {
 	XSetIconName(XtDisplay(TopLevel),XtWindow(TopLevel),name);
 	XFlush(XtDisplay(TopLevel));
	OldName = name;
    }
}
 

void
xmIconCreate()
{
    xmSetIconAndName(InitIcon);
    if (app_resources.iconGeometry != NIL(char)) {
	int scr, x, y, junk;
  	Arg args[2];

	for(scr = 0;	/* yyuucchh */
	    XtScreen(TopLevel) != ScreenOfDisplay(XtDisplay(TopLevel), scr);
	    scr++);

	XGeometry(XtDisplay(TopLevel), scr, app_resources.iconGeometry,
		  "", 0, 0, 0, 0, 0, &x, &y, &junk, &junk);
	XtSetArg(args[0], XtNiconX, x);
	XtSetArg(args[1], XtNiconY, y);
	XtSetValues(TopLevel, args, XtNumber(args));
    }
    return;
}


/* 
 * create the normal and busy xrn cursors
 */

void xrnBusyCursor()
{
    BusyCursor(TopLevel, True);
    xmSetIconAndName(BusyIcon);

    return;
}

void xrnUnbusyCursor()
{
    UnbusyCursor(TopLevel, True);
    xmSetIconAndName(PrevIcon);

    return;
}

/*ARGSUSED*/
void CBbusyCursor(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    xrnBusyCursor();
    return;
}

/*ARGSUSED*/
void CBunbusyCursor(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    xrnUnbusyCursor();
    return;
}

/*
  A Command widget is passed in.  Its border width/shadow and related
  stuff is grown so that it's larger than other buttons (so that it is
  visibly the default button).
  */

void makeDefaultButton(widget)
    Widget widget;
{
    Dimension border_width, highlight_thickness;

    XtVaGetValues(widget,
		  XtNborderWidth, (XtArgVal) &border_width,
		  XtNhighlightThickness, (XtArgVal) &highlight_thickness,
		  0);

    border_width++;
    highlight_thickness++;

    XtVaSetValues(widget,
		  XtNborderWidth, (XtArgVal) border_width,
		  XtNhighlightThickness, (XtArgVal) highlight_thickness,
		  0);
}
