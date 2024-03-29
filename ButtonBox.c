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
#include "utils.h"
#include "xrn.h"
#include "ButtonBox.h"

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
			 (String)0);
#else
    w = XtVaCreateWidget(name, boxWidgetClass, parent,
			 XtNallowResize, True,
			 XtNresizeToPreferred, True,
			 XtNshowGrip, False,
			 XtNskipAdjust, True,
			 (String)0);
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
				XmNactivateCallback, callbacks, (String)0);
#else
    w = XtVaCreateManagedWidget(name, commandWidgetClass, parent,
				XtNcallback, callbacks, (String)0);
#endif

    return w;
}

void ButtonBoxDoneAdding(w)
    Widget w;
{
    if (XtIsRealized(XtParent(w)))
      XtRealizeWidget(w);
    XtManageChild(w);
}

void ButtonBoxEmpty(w)
     Widget w;
{
  WidgetList children;
  Cardinal num_children;
  
  XtVaGetValues(w, XtNchildren, &children,
		XtNnumChildren, &num_children, (String)0);

  while (num_children-- >= 1)
    XtDestroyWidget(children[num_children]);
}

void ButtonBoxDestroy(w)
    Widget w;
{
    XtDestroyWidget(w);
}
