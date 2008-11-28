#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Paned.h>

#include "config.h"
#include "InfoLine.h"

Widget InfoLineCreate(name, initial_text, parent)
    String name;
    String initial_text;
    Widget parent;
{
    Widget w;
    Dimension height;

    w = XtVaCreateManagedWidget(name, labelWidgetClass, parent,
				XtNskipAdjust, True,
				XtNshowGrip, False,
				0);

    if (initial_text)
	XtVaSetValues(w, XtNlabel, initial_text, 0);

    XtVaGetValues(w, XtNheight, &height, 0);

    XtVaSetValues(w,
		  XtNmin, height,
		  XtNmax, height,
		  XtNpreferredPaneSize, height,
		  XtNresizeToPreferred, True,
		  0);

    return(w);
}

void InfoLineSet(w, text)
    Widget w;
    String text;
{
    XtVaSetValues(w, XtNlabel, text, 0);
}

void InfoLineDestroy(widget)
    Widget widget;
{
    XtDestroyWidget(widget);
}
