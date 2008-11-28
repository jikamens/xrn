#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/cancel.c,v 1.5 1994-11-18 14:21:00 jik Exp $";
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
 * cancel.c: cancel search
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#ifndef MOTIF
#include <X11/Xaw/Command.h>
#else
#include <Xm/PushB.h>
#endif

#include "xthelper.h"
#include "resources.h"
#include "xrn.h"
#include "xmisc.h"
#include "buttons.h"

static Widget CancelTopLevel  = (Widget) 0;

static void cancelButton _ARGUMENTS((Widget, XtPointer, XtPointer));

/*ARGSUSED*/
static void cancelButton(widget, closure, call_data)
    Widget widget;
    XtPointer closure, call_data;
{
    if (CancelTopLevel) {
	XtPopdown(CancelTopLevel);
	XtDestroyWidget(CancelTopLevel);
	CancelTopLevel = (Widget) 0;
    }
    abortSet();
    return;
}


static XtCallbackRec cancelCallbacks[] = {
    {(XtCallbackProc) cancelButton, 0}, {0, 0}
};

static Arg cancelArgs[] = {
    {XtNname, (XtArgVal) "cancel"},
    {MyNcallback, (XtArgVal) cancelCallbacks}
};


void cancelCreate()
{
    static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
	{XtNsaveUnder, (XtArgVal) False},
#ifdef notdef
	{XtNheight,  (XtArgVal) 50},
	{XtNwidth,  (XtArgVal) 350},
#endif
    };

    CancelTopLevel = XtCreatePopupShell("Cancel", transientShellWidgetClass,
					  TopLevel, shellArgs, XtNumber(shellArgs));

#ifndef MOTIF
    XtCreateManagedWidget("cancel", commandWidgetClass, CancelTopLevel,
			  cancelArgs, XtNumber(cancelArgs));
#else
    XtCreateManagedWidget("cancel", xmPushButtonWidgetClass, CancelTopLevel,
			  cancelArgs, XtNumber(cancelArgs));
#endif    
    XtRealizeWidget(CancelTopLevel);
    xthCenterWidgetOverCursor(CancelTopLevel);
    XtPopup(CancelTopLevel, XtGrabNone);
#ifdef MOTIF
/* Do this to make the label text appear */
    XmUpdateDisplay(CancelTopLevel);
#endif
    return;
}

void cancelDestroy()
{
    if (CancelTopLevel != 0) {
	XtPopdown(CancelTopLevel);
	XtDestroyWidget(CancelTopLevel);
	CancelTopLevel = 0;
    }
    return;
}
