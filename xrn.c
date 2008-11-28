#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: xrn.c,v 1.23 1995-02-19 23:43:58 jik Exp $";
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
 * xrn.c: set up the main screens
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>

#include "news.h"
#include "xthelper.h"
#include "xmisc.h"
#include "resources.h"
#include "internals.h"
#include "error_hnds.h"
#include "buttons.h"
#include "mesg.h"
#include "xrn.h"
#include "compose.h"
#include "mesg_strings.h"
#include "Text.h"
#include "ngMode.h"

#ifdef XFILESEARCHPATH
static void AddPathToSearchPath _ARGUMENTS((char *));
#endif

/* global variables that represent the widgets that are dynamically changed */

Widget TopLevel;
XtAppContext TopContext;
Widget Frame;
Widget TopButtonBox;    /* button box containing the command buttons */
Widget BottomButtonBox; /* button box containing the article specific buttons */
Widget TopInfoLine;      /* top button info line                      */
Widget BottomInfoLine;   /* bottom button info line                   */
Widget Text;             /* newsgroup and article subject display     */
Widget ArticleText;      /* article display                           */

int XRNState;            /* XRN status: news and x                    */

int inchannel, outchannel;

/*ARGSUSED*/
int main(argc, argv)
    int argc;
    char **argv;
{
    static Arg frameArgs[] = {			/* main window description */
	{XtNx, (XtArgVal) 10},
	{XtNy, (XtArgVal) 10},
	{XtNheight, (XtArgVal) 800},
	{XtNwidth, (XtArgVal) 680},
    };
#ifdef TITLEBAR
    Widget titlebar;
#endif
    static Arg labelArgs[] = {
	{XtNlabel, (XtArgVal) ""},
    };
    static Arg boxArgs[] = {
	/* Only the button boxes should resizeToPreferred */
	{XtNresizeToPreferred, (XtArgVal) True},
	{XtNskipAdjust, (XtArgVal) True}, /* skipAdjust DOESN'T REALLY EXIST */
    };
#if 0
    /* See below for why these are disabled. */
    XtWidgetGeometry intended, return_geometry;
    Arg sizeArgs[2];
#endif
    Arg fontArgs[1];

    int sv[2];
    Dimension height;

    pipe (sv);
    inchannel = sv[0];
    outchannel = sv[1];



    XRNState = 0;

#ifdef XFILESEARCHPATH
    AddPathToSearchPath(XFILESEARCHPATH);
#endif
    
    TopLevel = Initialize(argc, argv);

    ehInstallSignalHandlers();
    ehInstallErrorHandlers();

    if (app_resources.geometry != NIL(char)) {
	int bmask;
	bmask = XParseGeometry(app_resources.geometry,       /* geometry specification */
			       (int *) &frameArgs[0].value,    /* x      */
			       (int *) &frameArgs[1].value,    /* y      */
			       (unsigned int *) &frameArgs[3].value,    /* width  */
			       (unsigned int *) &frameArgs[2].value);   /* height */

	/* handle negative x and y values */
	if ((bmask & XNegative) == XNegative) {
	    frameArgs[0].value += (XtArgVal) DisplayWidth(XtDisplay(TopLevel),
							  DefaultScreen(XtDisplay(TopLevel)));
	    frameArgs[0].value -= (int) frameArgs[3].value;
	}
	if ((bmask & YNegative) == YNegative) {
	    frameArgs[1].value += (XtArgVal) DisplayHeight(XtDisplay(TopLevel),
							   DefaultScreen(XtDisplay(TopLevel)));
	    frameArgs[1].value -= (int) frameArgs[2].value;
	}
    }
    
    /* create the pane and its widgets */

    Frame = XtCreateManagedWidget("vpane", panedWidgetClass, TopLevel,
				  frameArgs, XtNumber(frameArgs));

#ifdef TITLEBAR
    XtSetArg(labelArgs[0], XtNlabel, app_resources.title);
    titlebar = XtCreateManagedWidget("titlebar", labelWidgetClass, Frame,
				     labelArgs, XtNumber(labelArgs));
#endif /* TITLEBAR */

    XtSetArg(labelArgs[0], XtNlabel, "");

    Text = TextCreate("index", True, Frame);
    TextSetLines(Text, app_resources.topLines);
    TextSetLineSelections(Text);

    TopInfoLine = XtCreateManagedWidget("indexinfo", labelWidgetClass, Frame,
					labelArgs, XtNumber(labelArgs));

    TopButtonBox = XtCreateManagedWidget("indexbuttons", boxWidgetClass, Frame,
					  boxArgs, XtNumber(boxArgs));

    /* article display text window */
    ArticleText = TextCreate("articleText", True, Frame);

    BottomInfoLine = XtCreateManagedWidget("textinfo", labelWidgetClass, Frame,
					   labelArgs, XtNumber(labelArgs));

    BottomButtonBox = XtCreateManagedWidget("textbuttons", boxWidgetClass, Frame,
					     boxArgs, XtNumber(boxArgs));

    createButtons();

    /* create the icon */
    xmIconCreate();

#ifdef TITLEBAR
    XtSetArg(fontArgs[0], XtNheight, (XtPointer) &height);
    XtGetValues(titlebar, fontArgs, XtNumber(fontArgs));
    XawPanedSetMinMax(titlebar, (int) height, (int) height);
#endif /* TITLEBAR */

    XtSetArg(fontArgs[0], XtNheight, &height);
    XtGetValues(TopInfoLine, fontArgs, XtNumber(fontArgs));
    XawPanedSetMinMax(TopInfoLine, (int) height, (int) height);
    
    XtSetArg(fontArgs[0], XtNheight, &height);
    XtGetValues(BottomInfoLine, fontArgs, XtNumber(fontArgs));
    XawPanedSetMinMax(BottomInfoLine, (int) height, (int) height);
    XtSetKeyboardFocus(Frame, ArticleText);
    
    /*
     * This next call doesn't do anything by default, unless you
     * modify the application defaults file, because there are no
     * accelerators for Text in it.  However, it makes it possible for
     * users to add accelerators to their own resources, so that, for
     * example, they can use the arrow keys to scroll through the
     * index window rather than through the article text.  To do that,
     * they would put the following in their resources:
     *
     * xrn*index.accelerators: #override \n\
     * 		<Key>Down:	next-line() \n\
     * 		<Key>Up:	previous-line()
     */
    XtInstallAccelerators(ArticleText, Text);

#if XtSpecificationRelease > 5
    XtAddCallback(TopLevel, XtNsaveCallback, saveNewsrcCB, NULL);
    XtAddCallback(TopLevel, XtNdieCallback, ehDieCB, NULL);
#endif /* X11R6 or greater */

    /*
      I'm #if 0'ing this code out, because as far as I can tell, now
      that I've moved things around so that initializeNews() and
      determineMode() get called after the main window is already
      realized, this code is no longer necessary.  Disabling it causes
      no problems under X11R6; if it causes problems for you under
      X11R5 or X11R4, let me know. -- jik 11/13/94
      */
#if 0
    /* Get the top button box to start out the right size. This is a */
    /* somewhat gross hack, but it does do the job.		     */
    intended.request_mode = CWWidth | XtCWQueryOnly;
    XtSetArg(sizeArgs[0], XtNwidth, &intended.width);
    XtGetValues(TopButtonBox, sizeArgs, 1);
    XtQueryGeometry(TopButtonBox, &intended, &return_geometry);
    XtSetArg(sizeArgs[0], XtNheight, return_geometry.height);
    XtSetValues(TopButtonBox, sizeArgs, 1);
    /* Let's do a similar gross hack for the bottom button box */
    XtSetArg(sizeArgs[0], XtNwidth, &intended.width);
    XtGetValues(BottomButtonBox, sizeArgs, 1);
    XtQueryGeometry(BottomButtonBox, &intended, &return_geometry);
    XtSetArg(sizeArgs[0], XtNheight, return_geometry.height);
    XtSetValues(BottomButtonBox, sizeArgs, 1);
#endif
    
    XtRealizeWidget(TopLevel);
    XRNState |= XRN_X_UP;
    xthWaitForMapped(TopLevel);

    /* initialize the news system, read the newsrc file */
    initializeNews();
    XRNState |= XRN_NEWS_UP;

    /* set up the text window, mode buttons, and question */
    determineMode();

    unbusyCursor();
    addTimeOut();

    if (app_resources.version == 0) {
	mesgPane(XRN_SERIOUS, 0, NO_APP_DEFAULTS_MSG);
    } else if (strcmp(app_resources.version, XRN_VERSION) != 0) {
	mesgPane(XRN_SERIOUS, 0, NO_APP_DEFAULTS_MSG);
	mesgPane(XRN_SERIOUS | XRN_APPEND, 0, VERSIONS_MSG, app_resources.version,
		 XRN_VERSION);
    }

    XtAppAddInput(TopContext,
                inchannel, (XtPointer) XtInputReadMask, processMessage, (XtPointer) 0);

    XtAppMainLoop(TopContext);
    exit(0);
}       

#ifdef XFILESEARCHPATH
static void AddPathToSearchPath(path)
    char *path;
{
     char *old, *new;

     old = getenv("XFILESEARCHPATH");
     if (old) {
#ifdef USE_PUTENV
	  /* +1 for =, +2 for :, +3 for null */
	  new = XtMalloc((Cardinal) (strlen("XFILESEARCHPATH") +
				     strlen(old) +
				     strlen(path) + 3));
	  (void) strcpy(new, "XFILESEARCHPATH");
	  (void) strcat(new, "=");
	  (void) strcat(new, old);
	  (void) strcat(new, ":");
	  (void) strcat(new, path);
	  putenv(new);
#else
	  /* +1 for colon, +2 for null */
	  new = XtMalloc((Cardinal) (strlen(old) + strlen(path) + 2));
	  (void) strcpy(new, old);
	  (void) strcat(new, ":");
	  (void) strcat(new, path);
	  setenv("XFILESEARCHPATH", new, 1);
#endif
     }
     else {
#ifdef USE_PUTENV
	  new = XtMalloc((Cardinal) (strlen("XFILESEARCHPATH") +
				     strlen(path) + 2));
	  (void) strcpy(new, "XFILESEARCHPATH");
	  (void) strcat(new, "=");
	  (void) strcat(new, path);
	  putenv(new);
#else
	  setenv("XFILESEARCHPATH", path, 1);
#endif
     }
}
#endif

