

#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: mesg.c,v 1.21 1995-01-25 03:17:52 jik Exp $";
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
 * mesg.c: message box
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>

#include "xthelper.h"
#include "resources.h"
#include "xrn.h"
#include "mesg.h"
#include "buttons.h"
#include "xmisc.h"
#include "butdefs.h"
#ifdef XRN_USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "ngMode.h"
#include "Text.h"

char error_buffer[2048];
/* entire widget */
static Widget MesgTopLevel = (Widget) 0;
/* text window */
static Widget MesgText = (Widget) 0;
/* amount of text currently in the window */
static long current_length = 0;
static char *MesgString = 0;

#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif

#define MESG_SIZE 4096
/*
 * If you have a window larger that 512 characters across, or there is
 * an info message to be displayed that is longer than 512 characters,
 * then someone should be shot!
 */
static char InfoString[512]; 

BUTTON(mesgDismiss,dismiss);
BUTTON(mesgClear,clear);

/*ARGSUSED*/
void mesgDismissFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XtPopdown(MesgTopLevel);
    TextDestroy(MesgText);
    XtDestroyWidget(MesgTopLevel);
    MesgTopLevel = (Widget) 0;
    MesgText = (Widget) 0;
    current_length = 0;
    return;
}

void mesgClearFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    TextClear(MesgText);
    current_length = 0;
    return;
}

static void displayMesgString _ARGUMENTS((char *new_string));

static void displayMesgString(new_string)
    char *new_string;
{
    long newlen = strlen(new_string);

    if (! MesgText)
	return;

    TextReplace(MesgText, new_string, newlen, current_length, current_length);
    current_length += newlen;
    TextSetInsertionPoint(MesgText, current_length);
}


int newMesgPaneName()
{
    static int number = 0;

    number++;
    if (! number)
	number++;

    return number;
}


/*ARGSUSED*/
/*VARARGS2*/
#ifdef XRN_USE_STDARG
void
mesgPane(int type, int name, char *fmtString, ...)
#else
void
mesgPane(type, name, fmtString, va_alist)
int type, name;		/* XRN_INFO, XRN_SERIOUS */
char *fmtString;
va_dcl
#endif /* XRN_USE_STDARG */
/*
 * brings up a new vertical pane, not moded
 *
 * the pane consists of 3 parts: title bar, scrollable text window,
 * button box
 */
{
    va_list args;
    Widget pane, buttonBox, label, button, clear_button;
    Arg fontArgs[1];
    Arg bargs[1];
    static int last_name = 0;
    static Arg labelArgs[] = {
	{XtNskipAdjust, (XtArgVal) True},
    };
    static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
    };
    time_t tm;
    Dimension height;
    char *time_str;
    char addBuff[MESG_SIZE];

    if (name && last_name && (name == last_name))
	type |= XRN_APPEND;
    last_name = name;

#ifdef XRN_USE_STDARG
    va_start(args, fmtString);
#else    
    va_start(args);
#endif

    tm = time(0);

    if ((XRNState & XRN_X_UP) != XRN_X_UP) {
	(void) fprintf(stderr, "%-24.24s: ", ctime(&tm));
	(void) vfprintf(stderr, fmtString, args);
	(void) fprintf(stderr, "\n");
	return;
    }

    if ((type & XRN_INFO) && (app_resources.info == False)) {
	(void) vsprintf(InfoString, fmtString, args);
	infoNow(InfoString);
	return;
    }

    time_str = ctime(&tm);
    time_str += 11; /* Skip over the day and date */
    time_str[8] = '\0'; /* We only want the time, not the year and the newline */

    if (MesgTopLevel == (Widget) 0) {
	static char *accel = "#override\n\
			<Key>0xff0a: set() notify() unset()\n\
			<Key>0xff0d: set() notify() unset()";

	MesgTopLevel = XtCreatePopupShell("Information", topLevelShellWidgetClass,
					  TopLevel, shellArgs, XtNumber(shellArgs));

	pane = XtVaCreateManagedWidget("pane", panedWidgetClass, MesgTopLevel,
				       NULL);

	label = XtCreateManagedWidget("label", labelWidgetClass, pane,
				      labelArgs, XtNumber(labelArgs));

	MesgText = TextCreate("text", True, pane);

	buttonBox = XtCreateManagedWidget("box", boxWidgetClass, 
					  pane, 0, 0);

	button = XtCreateManagedWidget("dismiss", commandWidgetClass, buttonBox,
			      mesgDismissArgs, XtNumber(mesgDismissArgs));

	clear_button = XtCreateManagedWidget("clear", commandWidgetClass,
					     buttonBox, mesgClearArgs,
					     XtNumber(mesgClearArgs));

	makeDefaultButton(button);

	XtSetArg(bargs[0], XtNaccelerators, XtParseAcceleratorTable(accel));
	XtSetValues(button, bargs, XtNumber(bargs));

	XtRealizeWidget(MesgTopLevel);
#ifndef ACCELERATORBUG
	XtInstallAccelerators(MesgTopLevel, button);
	XtInstallAccelerators(pane, button);
	XtInstallAccelerators(MesgText, button);
	XtInstallAccelerators(buttonBox, button);
	XtInstallAccelerators(label, button);
#endif

	XtSetArg(fontArgs[0], XtNheight, &height);
	XtGetValues(label, fontArgs, XtNumber(fontArgs));
	XawPanedSetMinMax(label, (int) height, (int) height);
	XawPanedAllowResize(MesgText, True);
	
	XDefineCursor(XtDisplay(MesgTopLevel), XtWindow(MesgTopLevel),
		      XCreateFontCursor(XtDisplay(MesgTopLevel), XC_left_ptr));

	XtPopup(MesgTopLevel, XtGrabNone);
    }

    if (! (current_length || MesgString)) {
	(void) sprintf(addBuff, "%s: ", time_str);
    }
    else if (type & XRN_SAME_LINE) {
	*addBuff = '\0';
    }
    else if (type & XRN_APPEND) {
	(void) sprintf(addBuff, "\n%8s  ", "");
    }
    else {
	(void) sprintf(addBuff, "\n--------\n%s: ", time_str);
    }

    (void) vsprintf(&addBuff[strlen(addBuff)], fmtString, args);

    displayMesgString(addBuff);
    
    return;
}

/*
 * put an informational 'msg' into the top information label
 *
 * If 'now' is true, then process as many X events as possible to
 * force the message to be displayed immediately.
 */
void _info(msg, now)
    char *msg;
    Boolean now;
{
    static char label[LABEL_SIZE] = "";
    static Boolean in_info = False;

    if (! in_info) {
	in_info = True;

	if ((XRNState && XRN_X_UP) == XRN_X_UP) {
	    if (strcmp(msg, label)) {
		XtVaSetValues(TopInfoLine, XtNlabel, (XtArgVal) msg, 0);
		(void)strncpy(label, msg, sizeof(label) - 1);
		label[sizeof(label) - 1] = '\0';

		if (now) {
		    xthHandlePendingExposeEvents();
		}
	    }
	}
	else {
	    (void) fprintf(stderr, "XRN: %s\n", msg);
	}

	in_info = False;
    }

    return;
}
