#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Command.h>

#include "xrn.h"

Widget ButtonBoxCreate(name, parent)
    String name;
    Widget parent;
{
    Widget w;

    w = XtVaCreateWidget(name, boxWidgetClass, parent,
			 XtNallowResize, True,
			 XtNresizeToPreferred, True,
			 XtNshowGrip, False,
			 XtNskipAdjust, True,
			 0);

    return w;
}

Widget ButtonBoxAddButton(name, callbacks, parent)
    String name;
    XtCallbackRec *callbacks;
    Widget parent;
{
    Widget w;

    w = XtVaCreateManagedWidget(name, commandWidgetClass, parent,
				XtNcallback, callbacks, 0);

    return w;
}

void ButtonBoxDoneAdding(w)
    Widget w;
{
    XtWidgetGeometry intended, ret;

    XtManageChild(w);

    /*
      I'm not really sure why this is necessary, but it is.
      */
    XtVaGetValues(w, XtNwidth, &intended.width, 0);
    intended.request_mode = CWWidth | XtCWQueryOnly;
    XtQueryGeometry(w, &intended, &ret);
    XtVaSetValues(w, XtNheight, ret.height, 0);
}

void ButtonBoxDestroy(w)
    Widget w;
{
    XtDestroyWidget(w);
}
