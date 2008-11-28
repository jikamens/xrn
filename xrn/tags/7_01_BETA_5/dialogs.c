
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: dialogs.c,v 1.10 1994-11-23 01:47:18 jik Exp $";
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
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#ifndef MOTIF
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#else
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#endif

#include "xthelper.h"
#include "xmisc.h"
#include "xrn.h"
#include "dialogs.h"

#ifdef VMS
Widget MyNameToWidget();
#define XtNameToWidget MyNameToWidget
#endif /* VMS */


#if defined(MOTIF)
/**********************************************************************
Called when return is pressed in the text field.  When this happens,
simulate pressing of the default button.
**********************************************************************/

static void returnHandler _ARGUMENTS((Widget, Widget,
				      XmPushButtonCallbackStruct *));

static void returnHandler(w, data, call_data)
    Widget w;
    Widget data;
    XmPushButtonCallbackStruct *call_data;
{
  XtCallActionProc(data, "ArmAndActivate", call_data->event, 0, 0);
}
#endif

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
#ifdef MOTIF
    Widget buttonBox, textf, lastButton;
    XmString xs;
    WidgetList button_list = (WidgetList)XtMalloc(sizeof(Widget) * count);
#else
    Widget typein;
#endif

    XtSetArg(shellArgs[2], XtNtransientFor, GetAncestorShell(parent));
    /* override does not get titlebar, transient does */
    popup = XtCreatePopupShell("popup", transientShellWidgetClass, parent,
			       shellArgs, XtNumber(shellArgs));
    
    /* create the dialog box */
#ifndef MOTIF
    XtSetArg(dargs[cnt], XtNvalue, textField); cnt++;
    XtSetArg(dargs[cnt], XtNlabel, title); cnt++;
    XtSetArg(dargs[cnt], XtNinput, True); cnt++;
    dialog = XtCreateManagedWidget("dialog", dialogWidgetClass, popup, dargs, cnt);
#else
    xs = XmStringCreateLtoR(title, XmSTRING_DEFAULT_CHARSET);
    dialog = XmCreateRowColumn(popup, "dialog", dargs, cnt);
    
    cnt = 0;
    XtSetArg(dargs[cnt], XmNlabelString, xs); cnt++;
    XtSetArg(dargs[cnt], XmNalignment, XmALIGNMENT_BEGINNING); cnt++;
    XtManageChild(XmCreateLabel(dialog, "label", dargs, cnt));
    XmStringFree(xs);
    
    if (textField != DIALOG_NOTEXT) {
      cnt = 0;
      XtSetArg(dargs[cnt], XmNvalue, textField); cnt++;
      textf = XmCreateText(dialog, "textField", dargs, cnt);
      XtManageChild(textf);
    }

    cnt = 0;
    buttonBox = XmCreateRowColumn(dialog, "buttons", dargs, cnt);
#endif

    /* add the buttons */
    for (i = 0; i < count; i++) {
#ifdef MOTIF
	Widget button;
#else
	Arg bargs[2];
#endif
	static XtCallbackRec callbacks[] = {
	    {CBbusyCursor, NULL},
	    {NULL, NULL},
	    {CBunbusyCursor, NULL},
	    {NULL, NULL},
	};

	callbacks[1].callback = args[i].handler;
	callbacks[1].closure = args[i].data;

#ifndef MOTIF
	XtSetArg(bargs[0], XtNlabel, args[i].buttonName);
	XtSetArg(bargs[1], XtNcallback, callbacks);
	if (i == count - 1) {
	    bb  = XtCreateManagedWidget("default", commandWidgetClass,
					dialog, bargs, XtNumber(bargs));
	} else {
	     XtCreateManagedWidget("command", commandWidgetClass,
				   dialog, bargs, XtNumber(bargs));
	}
#else
	{
	    static XtCallbackRec callback1[] = {
	       {NULL, NULL},
	       {NULL, NULL},
	    };
	    static XtCallbackRec callback2[] = {
	       {NULL, NULL},
	       {NULL, NULL},
	    };
	    static XtCallbackRec callback3[] = {
	       {NULL, NULL},
	       {NULL, NULL},
	    };
	    Arg margs[10];
	    int ct = 0;

	    xs = XmStringCreate(args[i].buttonName, XmSTRING_DEFAULT_CHARSET);
	    XtSetArg(margs[ct], XmNlabelString, xs);  ct++;
	    callback1[0] = callbacks[0];
	    XtSetArg(margs[ct], XmNarmCallback, callback1);  ct++;
	    callback2[0] = callbacks[1];
	    XtSetArg(margs[ct], XmNactivateCallback, callback2); ct++;
	    callback3[0] = callbacks[2];
	    XtSetArg(margs[ct], XmNdisarmCallback, callback3);  ct++;
	    callback3[0] = callbacks[2];
	    XtSetArg(margs[ct], XmNdisarmCallback, callback3);  ct++;
	    if (i == count - 1) {
	        bb = button = XmCreatePushButton(buttonBox, "default", margs,
						 ct);
	    } else {
	        button = XmCreatePushButton(buttonBox, "command", margs, ct);
	    }

	    lastButton = button_list[i] = button;
	    XmStringFree(xs);
	}
#endif
    }

#ifndef MOTIF
    if ((typein = XtNameToWidget(dialog, "value")) != 0) {
	XtSetKeyboardFocus(dialog, typein);
	XtSetKeyboardFocus(TopLevel, typein);
    }
#else
    XtManageChildren(button_list, count);
    XtFree((char *)button_list);
    XtManageChild(buttonBox);
    XtManageChild(dialog);
#endif
	
    makeDefaultButton(bb);

    XtRealizeWidget(popup);

#ifndef ACCELERATORBUG
    XtInstallAccelerators(dialog, bb);
    XtInstallAccelerators(popup, bb);
#endif
#ifndef MOTIF
    if (typein != 0) {
	XtInstallAccelerators(typein, bb);
	if (textField) {
	    XEvent ev;
	    /* force the text field to be 'big enough' */
	    XtCallActionProc(typein, "beginning-of-line", &ev, 0, 0);
	}
    }
#else
    if (textField != DIALOG_NOTEXT) {
      XtAddCallback(textf, XmNactivateCallback, (XtCallbackProc) returnHandler, (XtPointer) bb);
#if XmVersion >= 1001
      XmProcessTraversal(textf, XmTRAVERSE_CURRENT);
    } else {
      XmProcessTraversal(bb, XmTRAVERSE_CURRENT);
#else
      XtSetKeyboardFocus(dialog, textf);
    } else {
      XmProcessTraversal(dialog, bb);
#endif
    }
#endif
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
  
#ifndef MOTIF
    return XawDialogGetValueString(XtNameToWidget(popup, "dialog"));
#else
    char *result;

    result = XmTextGetString(XtNameToWidget(XtNameToWidget(popup, "dialog"),
					    "textField"));
    if (result) {
      return result;
    } else {
      return "";
    }
#endif
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
	args[0].buttonName = "no";
	
    if (button2)
	args[1].buttonName = button2;
    else
	args[1].buttonName = "yes";

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
