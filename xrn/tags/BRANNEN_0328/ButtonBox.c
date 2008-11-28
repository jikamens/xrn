#ifdef MOTIF
# include <Xm/Xm.h>
# include <Xm/PushB.h>
# include <Xm/RowColumn.h>
# include <Xm/PanedW.h>
#else
# include <X11/Intrinsic.h>
# include <X11/StringDefs.h>
# include <X11/Xaw/Box.h>
# include <X11/Xaw/Paned.h>
# include <X11/Xaw/Command.h>
#endif

#include "config.h"
#include "xrn.h"

Widget ButtonBoxCreate(name, parent)
    String name;
    Widget parent;
{
    Widget w;

#ifdef MOTIF
    w = XtVaCreateWidget(name, xmRowColumnWidgetClass, parent,
                         XmNpacking, XmPACK_TIGHT,
                         XmNorientation, XmHORIZONTAL,
			 XmNallowResize, True,
			 XmNskipAdjust, True,
                         /* nothing for motif here, i think - kb
			 XtNresizeToPreferred, True,
			 XtNshowGrip, False,
                         */
			 0);
#else
    w = XtVaCreateWidget(name, boxWidgetClass, parent,
			 XtNallowResize, True,
			 XtNresizeToPreferred, True,
			 XtNshowGrip, False,
			 XtNskipAdjust, True,
			 0);
#endif

    return w;
}

Widget ButtonBoxAddButton(name, callbacks, parent)
    String name;
    XtCallbackRec *callbacks;
    Widget parent;
{
    Widget w;

#ifdef MOTIF
    w = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,
				XmNactivateCallback, callbacks, 0);
#else
    w = XtVaCreateManagedWidget(name, commandWidgetClass, parent,
				XtNcallback, callbacks, 0);
#endif

    return w;
}

void ButtonBoxDoneAdding(w)
    Widget w;
{
    XtWidgetGeometry intended, ret;

    XtRealizeWidget(w);
    XtManageChild(w);

    /* kb - Now that the manager widget is realized (above), the manage method will have the
       correct size of all the children.  This works for Motif at least, and should work for
       Xaw.  But since I'm not testing Xaw, I'll leave the code below for now.  If it works
       for Xaw as well, then we can remove the geom. query code below.  If not, then more
       #ifdef MOTIF type lines will be needed.  If this is not enough of a "why", then check
       the Motif FAQ for a good explanation of geom. man. */
    /*
      I'm not really sure why this is necessary, but it is.
    XtVaGetValues(w, XtNwidth, &intended.width, 0);
    intended.request_mode = CWWidth | XtCWQueryOnly;
    XtQueryGeometry(w, &intended, &ret);
    XtVaSetValues(w, XtNheight, ret.height, 0);
     */
}

void ButtonBoxDestroy(w)
    Widget w;
{
    XtDestroyWidget(w);
}
