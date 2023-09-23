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
				(String)0);

    if (initial_text)
        InfoLineSet(w, initial_text);

    XtVaGetValues(w, XmNheight, &height, (String)0);

    XtVaSetValues(w,
		  XmNpaneMinimum, height,
		  XmNpaneMaximum, height,
		  (String)0);
#else
    w = XtVaCreateManagedWidget(name, labelWidgetClass, parent,
				XtNskipAdjust, True,
				XtNshowGrip, False,
				(String)0);

    if (initial_text)
        InfoLineSet(w, initial_text);

    XtVaGetValues(w, XtNheight, &height, (String)0);

    XtVaSetValues(w,
		  XtNmin, height,
		  XtNmax, height,
		  XtNpreferredPaneSize, height,
		  XtNresizeToPreferred, True,
		  (String)0);
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
    XtVaSetValues(w, XmNlabelString, x, (String)0);
    XmStringFree(x);
#else
    XtVaSetValues(w, XtNlabel, newString, (String)0);
#endif
    XtFree(newString);
}

void InfoLineDestroy(widget)
    Widget widget;
{
    XtDestroyWidget(widget);
}
