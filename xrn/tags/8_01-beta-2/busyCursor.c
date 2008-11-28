/*
  Functions to install a "I'm busy" cursor on all widgets in a tree of
  widgets, preserving the original cursor values so that they can be
  restored later.
  */

/*
  XXX Is there a better way to get a widget's popup list than going into
  its core structure?  I can't find one.
  */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Simple.h>

/* for do_popups support */
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>

#ifdef TEST
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <stdio.h>
#endif

#include "busyCursor.h"
#ifdef XRN
#include "utils.h"
#ifndef TEST
#include "resources.h"
#endif
#endif

typedef struct _cursor_info {
    Widget widget;
    Cursor cursor;
} cursor_info, *cursor_list;

static cursor_list cursors = 0;
static int num_cursors = 0, cursor_size = 0;
static int current_position = 0;

static cursor_info *find_info _ARGUMENTS((Widget));

static cursor_info *find_info(widget)
    Widget widget;
{
    int original_position = current_position;

    for (; current_position < num_cursors; current_position++)
	if (cursors[current_position].widget == widget)
	    return(&cursors[current_position++]);

    for (current_position = 0;
	 current_position< original_position;
	 current_position ++)
	if (cursors[current_position].widget == widget)
	    return(&cursors[current_position++]);

    return 0;
}

static cursor_info *insert_info _ARGUMENTS((Widget));

static cursor_info *insert_info(widget)
    Widget widget;
{
    cursor_info *info_p;

    if (! (info_p = find_info(0))) {
	if (num_cursors == cursor_size) {
	    if (! cursor_size)
		cursor_size = 1;
	    else
		cursor_size *= 2;
	    cursors = (cursor_list) XtRealloc((char *) cursors,
					      cursor_size * sizeof(*cursors));
	}
	info_p = &cursors[num_cursors++];
	current_position = num_cursors;
    }

    info_p->widget = widget;
    info_p->cursor = 0;

    return info_p;
}

	
static Cursor get_cursor(root)
    Widget root;
{
    Cursor the_cursor;

#if !defined(TEST) && defined(XRN)
    XColor colors[2];
#endif

    the_cursor = XCreateFontCursor(XtDisplay(root), XC_watch);

#if !defined(TEST) && defined(XRN)
    colors[0].pixel = app_resources.pointer_foreground;
    colors[1].pixel = app_resources.pointer_background;
    XQueryColors(XtDisplay(root),
		 DefaultColormap(XtDisplay(root),
				 DefaultScreen(XtDisplay(root))),
		 colors, 2);
    XRecolorCursor(XtDisplay(root), the_cursor,
		   colors, colors+1);
#endif

    return the_cursor;
}


void BusyCursor(root, do_popups)
    Widget root;
    Boolean do_popups;
{
    static Cursor the_cursor = 0;
    cursor_info *info_p;
    WidgetList children;
    Cardinal num_children = 0;
    static int in_busy = 0;
    int i;
    CorePart *core;

    if (! the_cursor)
	the_cursor = get_cursor(root);

    if (find_info(root))
	/* It's already busy'd. */
	return;

    info_p = insert_info(root);

    XtVaGetValues(root,
		  XtNcursor, &info_p->cursor,
		  XtNchildren, &children,
		  XtNnumChildren, &num_children,
		  0);
#ifdef TEST
    printf("Busying 0x%x\n", root);
#endif
    XtVaSetValues(root, XtNcursor, the_cursor, 0);

    in_busy++;

    for (i = 0; i < num_children; i++)
	BusyCursor(children[i], do_popups);

    if (do_popups && XtIsSubclass(root, coreWidgetClass)) {
	core =  &((WidgetRec *)root)->core;
	for (i = 0; i < core->num_popups; i++)
	    BusyCursor(core->popup_list[i], do_popups);
    }

    in_busy--;

    if (! in_busy)
	XFlush(XtDisplay(root));
}

void UnbusyCursor(root, do_popups)
    Widget root;
    Boolean do_popups;
{
    cursor_info *info_p;
    static int in_unbusy = 0;
    WidgetList children;
    Cardinal num_children = 0;
    int i;
    CorePart *core;

    if (! (info_p = find_info(root)))
	/* It's not busy'd. */
	return;

#ifdef TEST
    printf("Unbusying 0x%x\n", root);
#endif
    XtVaSetValues(root, XtNcursor, info_p->cursor, 0);
    info_p->widget = 0;

    XtVaGetValues(root,
		  XtNchildren, &children,
		  XtNnumChildren, &num_children,
		  0);

    in_unbusy++;

    for (i = 0; i < num_children; i++)
	UnbusyCursor(children[i], do_popups);

    if (do_popups && XtIsSubclass(root, coreWidgetClass)) {
	core =  &((WidgetRec *)root)->core;
	for (i = 0; i < core->num_popups; i++)
	    UnbusyCursor(core->popup_list[i], do_popups);
    }

    in_unbusy--;

    if (! in_unbusy)
	XFlush(XtDisplay(root));
}

#ifdef TEST
Widget top;

void buttonCallback(widget, event, string, count)
    Widget widget;
    XEvent *event;
    String *string;
    Cardinal *count;
{
    static Boolean busyd = False;

    if (busyd)
	UnbusyCursor(top);
    else
	BusyCursor(top);

    busyd = !busyd;
}

XtCallbackRec callbacks[] = {{buttonCallback, 0}, {0, 0}};

int main(argc, argv)
    int argc;
    char *argv[];
{
    Widget pane, box, button, text;
    XtAppContext context;

    top = XtOpenApplication(&context, "Test", 0, 0,
			    &argc, argv, 0,
			    sessionShellWidgetClass, 0, 0);

    pane = XtCreateManagedWidget("pane", panedWidgetClass, top, 0, 0);
    box = XtCreateManagedWidget("box", boxWidgetClass, pane, 0, 0);
    button = XtVaCreateManagedWidget("button", commandWidgetClass, box,
				     XtNcallback, callbacks,
				     0);
    text = XtVaCreateManagedWidget("text", asciiTextWidgetClass, pane,
				   XtNstring, "This is a test string.",
				   0);

    XtRealizeWidget(top);
    XtAppMainLoop(context);

    return(0);
}
#endif /* TEST */
