#ifdef MOTIF
# include <Xm/Xm.h>
# include <Xm/Label.h>
#else
# include <X11/Intrinsic.h>
# include <X11/StringDefs.h>
# include <X11/Xaw/Label.h>
# include <X11/Xaw/Paned.h>
#endif

#include "config.h"
#include "InfoLine.h"

Widget InfoLineCreate(name, initial_text, parent)
    String name;
    String initial_text;
    Widget parent;
{
    Widget w;
    Dimension height;

#ifdef MOTIF
    w = XtVaCreateManagedWidget(name, xmLabelWidgetClass, parent,
				XmNskipAdjust, True,
				0);

    if (initial_text)
        InfoLineSet(w, initial_text);

    XtVaGetValues(w, XmNheight, &height, 0);

    XtVaSetValues(w,
		  XmNpaneMinimum, height,
		  XmNpaneMaximum, height,
		  0);
#else
    w = XtVaCreateManagedWidget(name, labelWidgetClass, parent,
				XtNskipAdjust, True,
				XtNshowGrip, False,
				0);

    if (initial_text)
        InfoLineSet(w, initial_text);

    XtVaGetValues(w, XtNheight, &height, 0);

    XtVaSetValues(w,
		  XtNmin, height,
		  XtNmax, height,
		  XtNpreferredPaneSize, height,
		  XtNresizeToPreferred, True,
		  0);
#endif

    return(w);
}

void InfoLineSet(w, text)
    Widget w;
    String text;
{
    char *newString = XtNewString(text), *ptr;
#ifdef MOTIF
    XmString x;
#endif

    /* Info lines are one line long, so they can't have newlines in them,
       and tabs just waste space. */
    for (ptr = strpbrk(newString, "\n\t"); ptr; ptr = strpbrk(newString, "\n\t"))
      *ptr = ' ';

#ifdef MOTIF
    /* Yes, I know this is an old function and not the preferred way, but for now...kb */
    x = XmStringCreateSimple(newString);
    XtVaSetValues(w, XmNlabelString, x, 0);
    XmStringFree(x);
#else
    XtVaSetValues(w, XtNlabel, newString, 0);
#endif
    XtFree(newString);
}

void InfoLineDestroy(widget)
    Widget widget;
{
    XtDestroyWidget(widget);
}
