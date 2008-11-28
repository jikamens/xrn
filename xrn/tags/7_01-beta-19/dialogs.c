
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: dialogs.c,v 1.13 1995-01-25 03:17:52 jik Exp $";
/* Modified 2/20/92 dbrooks@osf.org to clean up dialog layout */
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
 * dialogs.c: create simple moded dialog boxes
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include "mesg_strings.h"
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>

#include "xthelper.h"
#include "xmisc.h"
#include "xrn.h"
#include "dialogs.h"

/*
 * find the closest ancestor of w which is a shell
 */
static Widget GetAncestorShell _ARGUMENTS((Widget));

static Widget GetAncestorShell(w)
    Widget w;
{
    while (w && (! XtIsShell(w)))
	w = XtParent(w);

    return (w);
}

/*
 * create a simple moded dialog box
 */
Widget CreateDialog(parent, title, textField, args, count)
    Widget parent;           /* parent window                         */
    char *title;             /* title of the dialog box               */
    char *textField;	     /* default text field                    */
    struct DialogArg *args;  /* description of the buttons            */
    unsigned int count;      /* number of buttons                     */
{
    Widget popup, dialog;
    Arg dargs[4];
    int cnt = 0;
    int i;
    Widget bb;
    static Arg shellArgs[] = {
	{XtNallowShellResize, (XtArgVal) True},
	{XtNinput, (XtArgVal) True},
        {XtNtransientFor, (XtArgVal) NULL},
    };
    Widget typein;

    XtSetArg(shellArgs[2], XtNtransientFor, GetAncestorShell(parent));
    /* override does not get titlebar, transient does */
    popup = XtCreatePopupShell("popup", transientShellWidgetClass, parent,
			       shellArgs, XtNumber(shellArgs));
    
    /* create the dialog box */
    XtSetArg(dargs[cnt], XtNvalue, textField); cnt++;
    XtSetArg(dargs[cnt], XtNlabel, title); cnt++;
    XtSetArg(dargs[cnt], XtNinput, True); cnt++;
    dialog = XtCreateManagedWidget("dialog", dialogWidgetClass, popup, dargs, cnt);

    /* add the buttons */
    for (i = 0; i < count; i++) {
	Arg bargs[2];
	static XtCallbackRec callbacks[] = {
	    {CBbusyCursor, NULL},
	    {NULL, NULL},
	    {CBunbusyCursor, NULL},
	    {NULL, NULL},
	};

	callbacks[1].callback = args[i].handler;
	callbacks[1].closure = args[i].data;

	XtSetArg(bargs[0], XtNlabel, args[i].buttonName);
	XtSetArg(bargs[1], XtNcallback, callbacks);
	if (i == count - 1) {
	    bb  = XtCreateManagedWidget("default", commandWidgetClass,
					dialog, bargs, XtNumber(bargs));
	} else {
	     XtCreateManagedWidget("command", commandWidgetClass,
				   dialog, bargs, XtNumber(bargs));
	}
    }

    if ((typein = XtNameToWidget(dialog, "value")) != 0) {
	XtSetKeyboardFocus(dialog, typein);
	XtSetKeyboardFocus(TopLevel, typein);
    }
	
    makeDefaultButton(bb);

    XtRealizeWidget(popup);

#ifndef ACCELERATORBUG
    XtInstallAccelerators(dialog, bb);
    XtInstallAccelerators(popup, bb);
#endif
    if (typein != 0) {
	XtInstallAccelerators(typein, bb);
	if (textField) {
	    XEvent ev;
	    /* force the text field to be 'big enough' */
	    XtCallActionProc(typein, "beginning-of-line", &ev, 0, 0);
	}
    }
    return(popup);
}

#ifdef DECFOCUSPATCH
static void FocusPopUp _ARGUMENTS((Widget, XtPointer, XEvent *));

static void FocusPopUp(popup, data, event)
    Widget popup;
    XtPointer data;
    XEvent *event;
{
    if (event->type == MapNotify) {
	XSetInputFocus(XtDisplay(popup), XtWindow(popup),
		     RevertToPointerRoot,  CurrentTime);
	XtRemoveEventHandler(popup, XtAllEvents, True, FocusPopUp, 0);
    }
    return;
}
#endif

void PopUpDialog(popup)
    Widget popup;
{
    xthCenterWidgetOverCursor(popup);
    XtPopup(popup, XtGrabExclusive);
#ifdef DECFOCUSPATCH
    XtAddEventHandler(popup, StructureNotifyMask, False, FocusPopUp, 0);
#endif
    return;
}

/*
 * pop down the dialog (do not destroy, it will be used again)
 */
void PopDownDialog(dialog)
    Widget dialog;
{
    XtPopdown(dialog);
    XtDestroyWidget(dialog);
    return;
}

char * GetDialogValue(popup)
    Widget popup;
{
  
    return XawDialogGetValueString(XtNameToWidget(popup, "dialog"));
}

/*
 * simple confirmation box
 */

static int retVal;

static void cbHandler _ARGUMENTS((Widget, XtPointer, XtPointer));

/* ARGSUSED */
static void cbHandler(widget, client_data, call_data)
    Widget widget;
    XtPointer client_data;
    XtPointer call_data;
{
    retVal = (int) client_data;
    return;
}

/*
 * pop up a confirmation box and return either 'XRN_CB_ABORT' or
 * 'XRN_CB_CONTINUE'.  Either or both of the button1 and button2
 * arguments can be null; they default to "no" and "yes".  The second
 * button is the default.
 */
int ConfirmationBox(parent, message, button1, button2)
    Widget parent;
    char *message, *button1, *button2;
{
    /* This is static, despite the fact that making it static wastes
       memory because it isn't really needed in between invocations of
       the function, because some old C compilers won't allow
       aggregate initialization of automatic variables.  Lose, lose.

       If I didn't have to make this static, I'd make it automatic,
       put the "no" and "yes" initializers for the buttonName fields
       in the aggregate initialization, and only assign to buttonName
       below if button1 or button2 is non-null.
       */
    static struct DialogArg args[] = {
	{0, cbHandler, (XtPointer) XRN_CB_ABORT},
	{0, cbHandler, (XtPointer) XRN_CB_CONTINUE},
    };
    XEvent ev;
    Widget widget;
    XtAppContext app = XtWidgetToApplicationContext(parent);

    if (button1)
	args[0].buttonName = button1;
    else
      args[0].buttonName = NO_STRING;
	
    if (button2)
	args[1].buttonName = button2;
    else
      args[1].buttonName = YES_STRING;

    retVal = -1;

    widget = CreateDialog(parent, message, DIALOG_NOTEXT, args, XtNumber(args));
    PopUpDialog(widget);

    for(;;) {
	XtAppNextEvent(app, &ev);
	(void) XtDispatchEvent(&ev);
	if (retVal != -1) {
	    PopDownDialog(widget);
	    return(retVal);
	}
    }
}
