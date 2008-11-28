/* InfoDialog.c */

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

/* entire widget */
static Widget MesgTopLevel = (Widget) 0;
/* text window */
static Widget MesgText = (Widget) 0;
/* amount of text currently in the window */
static long current_length = 0;


long
MesgLength(void)
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
        /*
	XtSetKeyboardFocus(MesgTopLevel, MesgText);
	XtInstallAccelerators(MesgText, button);
        */

        /*
	XDefineCursor(XtDisplay(MesgTopLevel), XtWindow(MesgTopLevel),
		      XCreateFontCursor(XtDisplay(MesgTopLevel), XC_left_ptr));
        */
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

    if (! MesgText)
	return;

/*
#ifdef MOTIF
    XmTextInsert(MesgText, current_length, new_string);
    current_length += newlen;
    XmTextSetInsertionPosition(MesgText, current_length);
#else
#endif
*/
    TextReplace(MesgText, new_string, newlen, current_length, current_length);
    current_length += newlen;
    TextSetInsertionPoint(MesgText, current_length);
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
    /*
    XmTextSetString("");
    */
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
