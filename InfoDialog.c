/*
 * xrn - an X-based NNTP news reader
 *
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

#ifdef MOTIF
# include <Xm/Xm.h>
# include <Xm/PanedW.h>
# include <Xm/MessageB.h>
#else
# include <X11/Xos.h>
# include <X11/cursorfont.h>
# include <X11/StringDefs.h>
# include <X11/Intrinsic.h>
# include <X11/Shell.h>
# include <X11/Xaw/Paned.h>
# include <X11/Xaw/Command.h>
# include <X11/Xaw/Box.h>
#endif

#include "butdefs.h"
#include "ButtonBox.h"
#include "InfoLine.h"
#include "Text.h"
#include "InfoDialog.h"
#include "ngMode.h"

/* entire widget */
static Widget MesgTopLevel = (Widget) 0;
/* text window */
static Widget MesgText = (Widget) 0;
/* amount of text currently in the window */
static long current_length = 0;


long
MesgLength()
{
	return (current_length);
}

#ifdef MOTIF
static void mesgDismissXmCallback _ARGUMENTS((Widget,XtPointer,XtPointer));
static void mesgClearXmCallback _ARGUMENTS((Widget,XtPointer,XtPointer));
#else
static void mesgDismissFunction _ARGUMENTS((Widget,XEvent*,String*,Cardinal*));
static void mesgClearFunction _ARGUMENTS((Widget,XEvent*,String*,Cardinal*));
BUTTON(mesgDismiss,dismiss);
BUTTON(mesgClear,clear);
#endif

void
InfoDialogCreate(Widget parent)
{
	Widget pane, buttonBox, label, button;
	static Arg shellArgs[] = {
	{XtNinput, (XtArgVal) True},
	};

	if (MesgTopLevel)
		return;

#ifdef MOTIF
	MesgTopLevel = XmCreateTemplateDialog(parent, "Information", 
					  shellArgs, XtNumber(shellArgs));

	pane = XmCreatePanedWindow(MesgTopLevel, "pane", NULL, 0);
	label = InfoLineCreate("label", 0, pane);
	MesgText = TextCreate("text", True, pane);

        XtAddCallback(MesgTopLevel, XmNcancelCallback,
		      mesgDismissXmCallback, NULL);
        XtAddCallback(MesgTopLevel, XmNokCallback, mesgClearXmCallback, NULL);

	XtRealizeWidget(MesgTopLevel);
        XtManageChild(pane);
        XtManageChild(MesgTopLevel);
#else
	MesgTopLevel = XtCreatePopupShell("Information", topLevelShellWidgetClass,
					  parent, shellArgs, XtNumber(shellArgs));

	pane = XtVaCreateManagedWidget("pane", panedWidgetClass, MesgTopLevel,
				       NULL);

	label = InfoLineCreate("label", 0, pane);

	MesgText = TextCreate("text", True, pane);

	buttonBox = ButtonBoxCreate("box", pane);

	button = ButtonBoxAddButton("mesgDismiss", mesgDismissCallbacks,
				    buttonBox);
	makeDefaultButton(button);

	(void) ButtonBoxAddButton("mesgClear", mesgClearCallbacks,
				  buttonBox);

	ButtonBoxDoneAdding(buttonBox);

	XtRealizeWidget(MesgTopLevel);
	XtSetKeyboardFocus(MesgTopLevel, MesgText);
	XtInstallAccelerators(MesgText, button);

	XDefineCursor(XtDisplay(MesgTopLevel), XtWindow(MesgTopLevel),
		      XCreateFontCursor(XtDisplay(MesgTopLevel), XC_left_ptr));

	XtPopup(MesgTopLevel, XtGrabNone);
#endif
}


void displayMesgString(new_string)
    char *new_string;
{
    long newlen = strlen(new_string);
    long nl_position;

    if (! MesgText)
	return;

    TextReplace(MesgText, new_string, newlen, current_length, current_length);
    current_length += newlen;
    /* Put the cursor immediately after the final newline, to make
       sure that if any messages in the window are longer than its
       width and the window has line wrapping and horizontal scroll
       bars turned off, the text will still be lined up properly,
       i.e., the window won't scroll right with no obvious way to get
       back to the left margin. */
    if ((nl_position = TextSearch(MesgText, current_length,
				  TextSearchLeft, "\n")) > -1)
      TextSetInsertionPoint(MesgText, nl_position + 1);
    else
      TextSetInsertionPoint(MesgText, 0);
}

/*ARGSUSED*/
#ifdef MOTIF
static void mesgDismissXmCallback(widget, client, cbsp)
    Widget widget;
    XtPointer client;
    XtPointer cbsp;
{
    XtDestroyWidget(XtParent(MesgTopLevel));
    MesgTopLevel = (Widget) 0;
    MesgText = (Widget) 0;
    current_length = 0;
}

static void mesgClearXmCallback(widget, client, cbsp)
    Widget widget;
    XtPointer client;
    XtPointer cbsp;
{
    TextClear(MesgText);
    current_length = 0;
}

#else

static void mesgDismissFunction(widget, event, string, count)
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
}

static void mesgClearFunction(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    TextClear(MesgText);
    current_length = 0;
}
#endif
