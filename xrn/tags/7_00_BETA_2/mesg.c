

#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/mesg.c,v 1.5 1994-10-11 14:40:34 jik Exp $";
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
#endif

#include "xthelper.h"
#include "resources.h"
#include "xrn.h"
#include "mesg.h"
#include "buttons.h"
#include "xmisc.h"
#include "butdefs.h"
#if defined(__STDC__) && !defined(NOSTDHDRS)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

char error_buffer[2048];
/* entire widget */
static Widget MesgTopLevel = (Widget) 0;
/* text window */
static Widget MesgText = (Widget) 0;
#ifdef MOTIF
static Widget VSB = (Widget) 0;
#endif

#define MESG_SIZE 4096
static char MesgString[MESG_SIZE];
/*
 * If you have a window larger that 512 characters across, or there is
 * an info message to be displayed that is longer than 512 characters,
 * then someone should be shot!
 */
static char InfoString[512]; 

/* whether or not we should hold off redisplaying the pane */
static int delay_redisplay = 0;

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
    return;
}


static void displayMesgString()
{
     int newlen = strlen(MesgString) + 1;
     Arg args[2];
     
     if (delay_redisplay || (! MesgText))
	  return;

#ifndef MOTIF
     XawTextDisableRedisplay(MesgText);
     XtSetArg(args[0], XtNstring, MesgString);
     XtSetArg(args[1], XtNlength, newlen);
     XtSetValues(MesgText, args, 2);
     XawTextSetInsertionPoint(MesgText, (XawTextPosition) newlen);
     XawTextEnableRedisplay(MesgText);
#else
     XmTextSetString(MesgText, MesgString);
/* XmTextShowPosition puts the last line at the top.  Do better. */
     if (VSB) {
       int max, value, slider_size, increment, page_increment;
       XtSetArg(args[0], XmNmaximum, &max);
       XtGetValues(VSB, args, 1);
       XmScrollBarGetValues(VSB, &value, &slider_size, &increment,
			    &page_increment);
       XmScrollBarSetValues(VSB, max, slider_size, increment,
			    page_increment, True);
     }
     else {
       if (newlen > 1)
	 XmTextShowPosition(MesgText, (XmTextPosition)(newlen - 2));
     }
#endif
}

void mesgDisableRedisplay()
{
     delay_redisplay = 1;
     return;
}

void mesgEnableRedisplay()
{
     delay_redisplay = 0;
     displayMesgString();
     return;
}

/* maman@cosinus.inria.fr: wrong address to vsprintf() when compiled
 * with gcc.  */

/*ARGSUSED*/
/*VARARGS2*/
#if defined(__STDC__) && !defined(NOSTDHDRS)
void
mesgPane(int type, char *fmtString, ...)
#else
void
mesgPane(type, fmtString, va_alist)
int type;		/* XRN_INFO, XRN_SERIOUS */
char *fmtString;
#endif
#if !defined(__STDC__) || defined(NOSTDHDRS)
    va_dcl
#endif
/*
 * brings up a new vertical pane, not moded
 *
 * the pane consists of 3 parts: title bar, scrollable text window,
 * button box
 */
{
    va_list args;
    Widget pane, buttonBox, label, button;
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
    static Arg boxArgs[] = {
#ifndef MOTIF
	{XtNskipAdjust, (XtArgVal) True},
#else
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
	{XtNuseStringInPlace,  (XtArgVal) True},
#else
	{XmNvalue, (XtArgVal) MesgString},
#endif
    };
    time_t tm;
    char addBuff2[MESG_SIZE];
    Dimension height;

#if defined(__STDC__) && !defined(NOSTDHDRS)
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
#else
	pane = XmCreatePanedWindow(MesgTopLevel, "pane", NULL, 0);
	XtManageChild(pane);
#endif

	(void) vsprintf(addBuff2, fmtString, args);
	(void) sprintf(MesgString, "%-24.24s: %s", ctime(&tm), addBuff2);

#ifndef MOTIF
	label = XtCreateManagedWidget("label", labelWidgetClass, pane,
				      labelArgs, XtNumber(labelArgs));

	MesgText = XtCreateManagedWidget("text", asciiTextWidgetClass, 
					 pane, textArgs, XtNumber(textArgs));

	buttonBox = XtCreateManagedWidget("box", boxWidgetClass, 
					 pane, boxArgs, XtNumber(boxArgs));

	button = XtCreateManagedWidget("dismiss", commandWidgetClass, buttonBox,
			      dismissArgs, XtNumber(dismissArgs));
	
#else
	label = XmCreateLabel(pane, "label", labelArgs, XtNumber(labelArgs));
	XtManageChild(label);

	MesgText = XmCreateScrolledText(pane, "text",
					textArgs, XtNumber(textArgs));
	XtManageChild(MesgText);
	XtSetArg(bargs[0], XmNverticalScrollBar, &VSB);
	XtGetValues(XtParent(MesgText), bargs, 1);

	buttonBox = XmCreateRowColumn(pane, "box", boxArgs, XtNumber(boxArgs));
	XtManageChild(buttonBox);

	button = XmCreatePushButton(buttonBox, "dismiss", dismissArgs,
				    XtNumber(dismissArgs));
	XtManageChild(button);
#endif    
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
	{
	  Arg args[3];
	  
	  XtSetArg(args[0], XmNpaneMaximum, height);
	  XtSetArg(args[1], XmNpaneMinimum, height);
	  XtSetArg(args[2], XmNallowResize, True);
	  XtSetValues(label, args, 3);
	}
	
#endif
	
	XDefineCursor(XtDisplay(MesgTopLevel), XtWindow(MesgTopLevel),
		      XCreateFontCursor(XtDisplay(MesgTopLevel), XC_left_ptr));

	XtPopup(MesgTopLevel, XtGrabNone);

	/* XawTextSetLastPos(MesgText, (XawTextPosition) utStrlen(MesgString)); */

	displayMesgString();
    } else {
	long len;
	long newlen;
	char addBuff[MESG_SIZE];

	(void) vsprintf(addBuff2, fmtString, args);
	(void) sprintf(addBuff, "%-24.24s: %s", ctime(&tm), addBuff2);
	len = strlen(MesgString);
	newlen = strlen(addBuff);

	if ((len + 10 + newlen) >= MESG_SIZE) {
	    (void) strcpy(MesgString, addBuff);
	    return;
	}
	else {
	     if (! (type & XRN_APPEND)) {
		  (void) strcat(&MesgString[len], "\n--------\n");
		  (void) strcat(&MesgString[len + 10], addBuff);
	     }
	     else {
		  (void) strcat(&MesgString[len], "\n");
		  (void) strcat(&MesgString[len + 1], addBuff);
	     }
	}

	displayMesgString();
    }
    
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
