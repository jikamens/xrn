/* Modified 2/20/92 dbrooks@osf.org to clean up dialog layout */

/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1988-1993, Ellen M. Sentovich and Rick L. Spickelmier.
 * Copyright (c) 1994-2023, Jonathan Kamens.
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
#include <X11/Xaw/Text.h>

#include "xthelper.h"
#include "xmisc.h"
#include "xrn.h"
#include "dialogs.h"

#ifdef XRN_USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
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
    Widget bb WALL(= 0);
    static Arg shellArgs[] = {
	{XtNallowShellResize, (XtArgVal) True},
	{XtNinput, (XtArgVal) True},
        {XtNtransientFor, (XtArgVal) NULL},
    };
    Widget typein;
    char *t = XtNewString(title), *p;

    p = t;
    while ((p = strchr(p, '\t')))
      *p = ' ';

    XtSetArg(shellArgs[2], XtNtransientFor, GetAncestorShell(parent));
    /* override does not get titlebar, transient does */
    popup = XtCreatePopupShell("popup", transientShellWidgetClass, parent,
			       shellArgs, XtNumber(shellArgs));
    
    /* create the dialog box */
    XtSetArg(dargs[cnt], XtNvalue, textField); cnt++;
    XtSetArg(dargs[cnt], XtNlabel, t); cnt++;
    XtSetArg(dargs[cnt], XtNinput, True); cnt++;
    dialog = XtCreateManagedWidget("dialog", dialogWidgetClass, popup, dargs, cnt);

    /* add the buttons */
    XtFree(t);
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
	    /* force the text field to be 'big enough' */
	    XtCallActionProc(typein, "beginning-of-line", 0, 0, 0);
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
    retVal = (POINTER_NUM_TYPE) client_data;
    return;
}

/*
 * pop up a confirmation box and return either 'XRN_CB_ABORT' or
 * 'XRN_CB_CONTINUE'.  Either or both of the button1 and button2
 * arguments can be null; they default to "no" and "yes".  The second
 * button is the default.
 *
 * Always returns XRN_CB_ABORT if X isn't up (according to the
 * XRN_X_UP bit in the XRNState variable).
 */
int ConfirmationBox(
		    _ANSIDECL(Widget,	parent),
		    _ANSIDECL(char *,	message),
		    _ANSIDECL(char *,	button1),
		    _ANSIDECL(char *,	button2),
		    _ANSIDECL(Boolean,	continue_first)
		    )
     _KNRDECL(Widget,	parent)
     _KNRDECL(char *,	message)
     _KNRDECL(char *,	button1)
     _KNRDECL(char *,	button2)
     _KNRDECL(Boolean,	continue_first)
{
  int retval;

  if (! (XRNState & XRN_X_UP))
    return XRN_CB_ABORT;

  retval = ChoiceBox(parent, message, 2,
		     button1 ? button1 : NO_STRING,
		     button2 ? button2 : YES_STRING);

  if (continue_first)
    return ((retval == 1) ? XRN_CB_CONTINUE : XRN_CB_ABORT);
  else
    return ((retval == 1) ? XRN_CB_ABORT : XRN_CB_CONTINUE);
}

/*
 *
 * Pop up a box with an arbitrary number of buttons, and return the
 * number of the button that was selected (starting the count at 1).
 * The last button in the array is the default, and will be selected
 * if the user hits return instead of clicking on one of the buttons.
 */
#ifdef XRN_USE_STDARG
int ChoiceBox(Widget parent, char *message, int count, ...)
#else
int ChoiceBox(parent, message, count, va_alist)
     Widget parent;
     char *message;
     int count;
     va_dcl
#endif /* XRN_USE_STDARG */
{
  struct DialogArg *dialog_args;
  XEvent ev;
  Widget widget;
  XtAppContext app = XtWidgetToApplicationContext(parent);
  POINTER_NUM_TYPE i;
  va_list args;

  dialog_args = (struct DialogArg *) XtMalloc(sizeof(*dialog_args) * count);

#ifdef XRN_USE_STDARG
  va_start(args, count);
#else
  va_start(args);
#endif

  for (i = 0; i < count; i++) {
    dialog_args[i].buttonName = va_arg(args, char *);
    dialog_args[i].handler = cbHandler;
    dialog_args[i].data = (XtPointer) (i + 1);
  }

  va_end(args);

  retVal = -1;

  widget = CreateDialog(parent, message, DIALOG_NOTEXT, dialog_args, count);

  PopUpDialog(widget);

  for (;;) {
    XtAppNextEvent(app, &ev);
    (void) MyDispatchEvent(&ev);
    if (retVal != -1) {
      PopDownDialog(widget);
      XtFree((char *) dialog_args);
      return(retVal);
    }
  }
}

static int password_result;
static char *dialog_password;

static void passwordHandler _ARGUMENTS((Widget, XtPointer,
					XtPointer));

static void passwordHandler(widget, client_data, call_data)
     Widget widget;
     XtPointer client_data, call_data;
{
  Widget dialog = XtParent(XtParent(widget));

  password_result = (POINTER_NUM_TYPE) client_data;
  if (password_result == XRN_CB_CONTINUE) {
    dialog_password = GetDialogValue(dialog);
    dialog_password = XtNewString(dialog_password);
  }
  PopDownDialog(dialog);
  return;
}

String PasswordBox(widget, prompt)
     Widget widget;
     String prompt;
{
  Widget dialog;
  static struct DialogArg args[] = {
    {ABORT_STRING, passwordHandler, (XtPointer) XRN_CB_ABORT},
    {DOIT_STRING, passwordHandler, (XtPointer) XRN_CB_CONTINUE},
  };

  password_result = -1;

  dialog = CreateDialog(TopLevel, prompt, DIALOG_TEXT, args, XtNumber(args));
  XtVaSetValues(XtNameToWidget(dialog, "dialog.value"), XtNecho, 0, (String)0);
  PopUpDialog(dialog);

  while (password_result < 0) {
    XEvent ev;

    XtAppNextEvent(TopContext, &ev);
    MyDispatchEvent(&ev);
  }

  if (password_result == XRN_CB_CONTINUE)
    return dialog_password;
  else
    return 0;
}
