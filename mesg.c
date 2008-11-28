

#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/mesg.c,v 1.13 1994-11-18 14:32:58 jik Exp $";
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

#ifndef MOTIF
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#else
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/ScrollBar.h>
#endif

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

char error_buffer[2048];
/* entire widget */
static Widget MesgTopLevel = (Widget) 0;
/* text window */
static Widget MesgText = (Widget) 0;
/* amount of text currently in the window */
static int current_length = 0;
static char *MesgString = 0;

#ifdef MOTIF
static Widget VSB = (Widget) 0;
#else
#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif
#endif /* MOTIF */

#define MESG_SIZE 4096
/*
 * If you have a window larger that 512 characters across, or there is
 * an info message to be displayed that is longer than 512 characters,
 * then someone should be shot!
 */
static char InfoString[512]; 

BUTTON(dismiss,dismiss);

/*ARGSUSED*/
static void dismissFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    XtPopdown(MesgTopLevel);
    XtDestroyWidget(MesgTopLevel);
    MesgTopLevel = (Widget) 0;
    MesgText = (Widget) 0;
    current_length = 0;
    return;
}

BUTTON(clear,clear);

static void clearFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
#ifdef MOTIF
    XmTextSetString(MesgText, "");
#else
    XtVaSetValues(MesgText, XtNstring, (XtArgVal) "", 0);
#endif
    current_length = 0;
    return;
}

static void displayMesgString _ARGUMENTS((char *new_string));

static void displayMesgString(new_string)
    char *new_string;
{
     int newlen;
#ifndef MOTIF
     XawTextBlock block;
#endif

     if (MesgString)
	 MesgString = XtRealloc(MesgString,
				strlen(MesgString) + strlen(new_string) + 1);
     else
	 MesgString = XtNewString(new_string);

     if (! MesgText)
	 return;

     newlen = strlen(MesgString);

#ifndef MOTIF
     block.firstPos = 0;
     block.length = newlen;
     block.ptr = MesgString;
     block.format = XawFmt8Bit;

     /*
       XXX Is there a better way to be able to append to a text widget
       without letting the user edit it, other than making it
       read-only by default, and append-only right when adding the
       text?  I can't find one.
       */
     (void) XtVaSetValues(MesgText, XtNeditType, (XtArgVal) XawtextAppend, 0);
     (void) XawTextReplace(MesgText, current_length, current_length, &block);
     (void) XtVaSetValues(MesgText, XtNeditType, (XtArgVal) XawtextRead, 0);
     FREE(MesgString);
     current_length += newlen;
     XawTextSetInsertionPoint(MesgText,
			      (XawTextPosition) current_length);
#else
     (void) XmTextInsert(MesgText, current_length, MesgString);
     FREE(MesgString);
     current_length += newlen;

/* XmTextShowPosition puts the last line at the top.  Do better. */
     if (VSB) {
       int max, value, slider_size, increment, page_increment;
       XtVaGetValues(VSB, XmNmaximum, (XtArgVal) &max, 0);
       XmScrollBarGetValues(VSB, &value, &slider_size, &increment,
			    &page_increment);
       XmScrollBarSetValues(VSB, max, slider_size, increment,
			    page_increment, True);
     }
     else {
       if (current_length > 1)
	 XmTextShowPosition(MesgText,
			    (XmTextPosition) (current_length - 2));
     }
#endif
}


/* maman@cosinus.inria.fr: wrong address to vsprintf() when compiled
 * with gcc.  */

/*ARGSUSED*/
/*VARARGS2*/
#ifdef XRN_USE_STDARG
void
mesgPane(int type, char *fmtString, ...)
#else
void
mesgPane(type, fmtString, va_alist)
int type;		/* XRN_INFO, XRN_SERIOUS */
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
    static Arg labelArgs[] = {
#ifndef MOTIF
	{XtNlabel, (XtArgVal) "Information (can be left up or dismissed)"},
	{XtNskipAdjust, (XtArgVal) True},
#else
/* XmNlabelString in app-defaults file */
	{XmNskipAdjust, (XtArgVal) True},
#endif
    };
    static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
    };
    static Arg textArgs[] = {
#ifndef MOTIF
#ifndef FIXED_XAW_TEXT_NULL_STRING_BUG
	{XtNstring, (XtArgVal) ""},
	{XtNlength, (XtArgVal) 1},
#endif
	{XtNtype,  (XtArgVal) XawAsciiString},
	{XtNeditType,  (XtArgVal) XawtextRead},
#else
	{XmNvalue, (XtArgVal) ""},
#endif
    };
    time_t tm;
    Dimension height;
    char *time_str;
    char addBuff[MESG_SIZE];

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
#ifndef MOTIF
	static char *accel = "#override\n\
			<Key>0xff0a: set() notify() unset()\n\
			<Key>0xff0d: set() notify() unset()";
#else
	static char *accel = "#override\n\
			<Key>osfSelect: ArmAndActivate()\n\
			<Key>osfActivate: ArmAndActivate()\n\
			~Shift~Meta~Alt<Key>Return: ArmAndActivate()";
#endif

	MesgTopLevel = XtCreatePopupShell("Information", topLevelShellWidgetClass,
					  TopLevel, shellArgs, XtNumber(shellArgs));

#ifndef MOTIF
	pane = XtVaCreateManagedWidget("pane", panedWidgetClass, MesgTopLevel,
				       NULL);

	label = XtCreateManagedWidget("label", labelWidgetClass, pane,
				      labelArgs, XtNumber(labelArgs));

	MesgText = XtCreateManagedWidget("text", asciiTextWidgetClass, 
					 pane, textArgs, XtNumber(textArgs));

	buttonBox = XtCreateManagedWidget("box", boxWidgetClass, 
					  pane, 0, 0);

	button = XtCreateManagedWidget("dismiss", commandWidgetClass, buttonBox,
			      dismissArgs, XtNumber(dismissArgs));

	clear_button = XtCreateManagedWidget("clear", commandWidgetClass,
					     buttonBox, clearArgs,
					     XtNumber(clearArgs));
#else
	pane = XmCreatePanedWindow(MesgTopLevel, "pane", NULL, 0);
	XtManageChild(pane);

	label = XmCreateLabel(pane, "label", labelArgs, XtNumber(labelArgs));
	XtManageChild(label);

	MesgText = XmCreateScrolledText(pane, "text",
					textArgs, XtNumber(textArgs));
	XtManageChild(MesgText);
	XtSetArg(bargs[0], XmNverticalScrollBar, &VSB);
	XtGetValues(XtParent(MesgText), bargs, 1);

	buttonBox = XmCreateRowColumn(pane, "box", 0, 0);
	XtManageChild(buttonBox);

	button = XmCreatePushButton(buttonBox, "dismiss", dismissArgs,
				    XtNumber(dismissArgs));
	XtManageChild(button);

	clear_button = XmCreatePushButton(buttonBox, "clear", clearArgs,
					  XtNumber(clearArgs));
        XtManageChild(clear_button);
#endif
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
#ifndef MOTIF
	XawPanedSetMinMax(label, (int) height, (int) height);
	XawPanedAllowResize(MesgText, True);
#else
	XtVaSetValues(label,
		      XmNpaneMinimum, height,
		      XmNpaneMaximum, height,
		      0);
	XtVaSetValues(MesgText, XmNallowResize, True, 0);
#endif
	
	XDefineCursor(XtDisplay(MesgTopLevel), XtWindow(MesgTopLevel),
		      XCreateFontCursor(XtDisplay(MesgTopLevel), XC_left_ptr));

	XtPopup(MesgTopLevel, XtGrabNone);
    }

    if (! (current_length || MesgString)) {
	(void) sprintf(addBuff, "%s: ", time_str);
    }
    else if (type & XRN_APPEND) {
	(void) sprintf(addBuff, "\n%8s  ", "");
    }
    else if (type & XRN_SAME_LINE) {
	*addBuff = '\0';
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
 */
void info(msg)
    char *msg;
{
    Arg infoLineArg[1];

    if ((XRNState && XRN_X_UP) == XRN_X_UP) {
#ifndef MOTIF
	XtSetArg(infoLineArg[0], XtNlabel, msg);
	XtSetValues(TopInfoLine, infoLineArg, XtNumber(infoLineArg));
#else
	XmString xs;

	xs = XmStringCreate(msg, XmSTRING_DEFAULT_CHARSET);
	XtSetArg(infoLineArg[0], XmNlabelString, xs);
	XtSetValues(TopInfoLine, infoLineArg, 1);
	XmStringFree(xs);
#endif
    } else {
	(void) fprintf(stderr, "XRN: %s\n", msg);
    }
    return;
}

/*
 * put an informational 'msg' into the top information label and force an update
 */
void infoNow(msg)
    char *msg;
{
    info(msg);
    if ((XRNState && XRN_X_UP) == XRN_X_UP) {
	xthHandleAllPendingEvents();
    }
    return;
}
