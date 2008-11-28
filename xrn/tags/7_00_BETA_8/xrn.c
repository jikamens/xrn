#if !defined(lint) && !defined(SABER)
static char XRNrcsid[] = "$Header: /d/src/cvsroot/xrn/xrn.c,v 1.9 1994-10-16 16:06:04 jik Exp $";
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

#ifndef MOTIF
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#else
#include <Xm/PanedW.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#endif

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

#ifdef MOTIF
#include "MotifXawHack.h"
#endif

#ifdef XFILESEARCHPATH
static void AddPathToSearchPath _ARGUMENTS((char *));
#endif

/* global variables that represent the widgets that are dynamically changed */

Widget TopLevel;
Widget Frame;
Widget TopButtonBox;    /* button box containing the command buttons */
Widget BottomButtonBox; /* button box containing the article specific buttons */
Widget TopInfoLine;      /* top button info line                      */
Widget BottomInfoLine;   /* bottom button info line                   */
Widget Text;             /* newsgroup and article subject display     */
Widget ArticleText;      /* article display                           */

#ifdef MOTIF
Widget ArticleTextText;		/* Motif text widget used for article text */
Widget ArticleTextList;		/* Motif list widget used for all groups */
Widget ArticleContainer;	/* Container for the above, only one shows */

static XmString emptyString;
#endif

int XRNState;            /* XRN status: news and x                    */

int inchannel, outchannel;

/*ARGSUSED*/
int main(argc, argv)
    int argc;
    char **argv;
{
    XtWidgetGeometry intended, return_geometry;
    static Arg frameArgs[] = {			/* main window description */
	{XtNx, (XtArgVal) 10},
	{XtNy, (XtArgVal) 10},
	{XtNheight, (XtArgVal) 800},
	{XtNwidth, (XtArgVal) 680},
    };
#ifdef TITLEBAR
    Widget titlebar;
    static Arg labelArgs[] = {
#ifndef MOTIF
	{XtNlabel, (XtArgVal) 0},
#else
	{XmNlabelString, (XtArgVal) 0},
	{XmNrecomputeSize, (XtArgVal) False},
	{XmNskipAdjust, (XtArgVal) True},
	{XmNallowResize, (XtArgVal) False},
#endif
    };
#endif
    static Arg genericArgs[] = {
#ifndef MOTIF
	{XtNskipAdjust, (XtArgVal) True}, /* skipAdjust DOESN'T REALLY EXIST */
#else
	{XmNlabelString, (XtArgVal) 0},
	{XmNrecomputeSize, (XtArgVal) False},
	{XmNskipAdjust, (XtArgVal) True},
	{XmNallowResize, (XtArgVal) False},
#endif
    };
    static Arg boxArgs[] = {
#ifndef MOTIF
	/* Only the button boxes should resizeToPreferred */
	{XtNresizeToPreferred, (XtArgVal) True},
	{XtNskipAdjust, (XtArgVal) True}, /* skipAdjust DOESN'T REALLY EXIST */
#else
	{XmNallowResize, (XtArgVal) True},
#endif
    };
#ifndef MOTIF
    static XawTextSelectType selarray[] = {XawselectLine, XawselectNull};
    static Arg textArgs[] = {		/* newsgroup/subject text window */
	{XtNstring,  (XtArgVal) ""},
	{XtNselectTypes, (XtArgVal) selarray },
	{XtNtype,  (XtArgVal) XawAsciiString},
	{XtNeditType,  (XtArgVal) XawtextRead},
	{XtNuseStringInPlace,  (XtArgVal) True},
    };
#else
    static Arg listArgs[] = {
      {XmNvisibleItemCount, (XtArgVal) TOPLINES},
      {XmNskipAdjust, (XtArgVal) False},
      {XmNallowResize, (XtArgVal) False},
      {XmNselectionPolicy, (XtArgVal) XmEXTENDED_SELECT},
    };
#endif

    Arg sizeArgs[2], fontArgs[1];

#ifndef MOTIF
    static Arg articleTextArgs[] = {	/* article/all text window */
	{XtNstring,  (XtArgVal) ""},
	{XtNselectTypes, (XtArgVal) selarray},
	{XtNtype,  (XtArgVal) XawAsciiString},
	{XtNeditType,  (XtArgVal) XawtextRead},
	{XtNuseStringInPlace,  (XtArgVal) True},
    };
#else
    static Arg articleTextArgs[] = {
        {XmNskipAdjust, (XtArgVal) False},
    };

    static Arg aListArgs[] = {
      {XmNselectionPolicy, (XtArgVal) XmEXTENDED_SELECT},
      {XmNpaneMinimum, (XtArgVal) 1},
      {XmNpaneMaximum, (XtArgVal) 1},
    };

    static Arg contArgs[] = {
      {XmNallowResize, (XtArgVal) False},
      {XmNskipAdjust, (XtArgVal) False},
      {XmNmarginHeight, (XtArgVal) 0},
      {XmNmarginWidth, (XtArgVal) 0},
      {XmNsashHeight, (XtArgVal) 1},
      {XmNsashWidth, (XtArgVal) 1},
      {XmNseparatorOn, (XtArgVal) False},
      {XmNspacing, (XtArgVal) 0},
    };

    static Arg sepArgs[] = {
      {XmNskipAdjust, (XtArgVal) True},
    };

#endif
    XFontStruct *textFont;
    Position topMargin;
    Position bottomMargin;
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

#ifndef MOTIF    
    Frame = XtCreateManagedWidget("vpane", panedWidgetClass, TopLevel,
				  frameArgs, XtNumber(frameArgs));
#else
    emptyString = XmStringCreate(" ", XmSTRING_DEFAULT_CHARSET);
    XtSetArg(genericArgs[0], XmNlabelString, emptyString);
    XtSetWarningHandler((XtErrorHandler) XawNothing);

    Frame = XmCreatePanedWindow(TopLevel, "vpane",
				  frameArgs, XtNumber(frameArgs));
    XtManageChild(Frame);
#endif
    
#ifdef TITLEBAR
#ifndef MOTIF
    XtSetArg(labelArgs[0], XtNlabel, app_resources.title);
    titlebar = XtCreateManagedWidget("titlebar", labelWidgetClass, Frame,
				     labelArgs, XtNumber(labelArgs));
#else
    {
      XmString xs;

      xs = XmStringCreate(app_resources.title, XmSTRING_DEFAULT_CHARSET);
      XtSetArg(labelArgs[0], XmNlabelString, xs);
      titlebar = XmCreateLabel(Frame, "titlebar",
			       labelArgs, XtNumber(labelArgs));
      XtManageChild(titlebar);
      XmStringFree(xs);
    }
#endif
#endif

#ifndef MOTIF
    Text = XtCreateManagedWidget("index", asciiTextWidgetClass, Frame,
					 textArgs, XtNumber(textArgs));
#else
    XtSetArg(listArgs[0], XmNvisibleItemCount, app_resources.topLines);
    Text = XmCreateScrolledList(Frame, "index", listArgs, XtNumber(listArgs));
    XtManageChild(Text);
    XtAddCallback(Text, XmNdefaultActionCallback, TextListSelection, (XtPointer) 0);
    XtAddCallback(Text, XmNextendedSelectionCallback, TextListSelection, (XtPointer) 0);
#endif

#ifndef MOTIF
    XtVaGetValues(Text,
		  XtNbottomMargin, &bottomMargin,
		  XtNtopMargin, &topMargin,
		  XtNfont, &textFont,
		  0);

    XtVaSetValues(Text, XtNheight,
		  (XtArgVal) ((Dimension)
			      (bottomMargin + topMargin +
			       app_resources.topLines *
			       (textFont->max_bounds.ascent +
				textFont->max_bounds.descent))),
		  0);
#endif
    
#ifndef MOTIF
    TopInfoLine = XtCreateManagedWidget("indexinfo", labelWidgetClass, Frame,
					genericArgs, XtNumber(genericArgs));

    TopButtonBox = XtCreateManagedWidget("indexbuttons", boxWidgetClass, Frame,
					  boxArgs, XtNumber(boxArgs));

    /* article display text window */
    ArticleText = XtCreateManagedWidget("articleText", asciiTextWidgetClass, Frame,
					 articleTextArgs, XtNumber(articleTextArgs));

    BottomInfoLine = XtCreateManagedWidget("textinfo", labelWidgetClass, Frame,
					   genericArgs, XtNumber(genericArgs));

    BottomButtonBox = XtCreateManagedWidget("textbuttons", boxWidgetClass, Frame,
					     boxArgs, XtNumber(boxArgs));
#else
    TopInfoLine = XmCreateLabel(Frame, "indexinfo",
				genericArgs, XtNumber(genericArgs));
    XtManageChild(TopInfoLine);
    
    XtManageChild(XmCreateSeparator(Frame, "separator",
				    sepArgs, XtNumber(sepArgs)));

    TopButtonBox = XmCreateRowColumn(Frame, "indexbuttons",
				     boxArgs, XtNumber(boxArgs));
    XtManageChild(TopButtonBox);

    XtManageChild(XmCreateSeparator(Frame, "separator",
				    sepArgs, XtNumber(sepArgs)));

    ArticleContainer = XmCreatePanedWindow(Frame, "articleText",
				    contArgs, XtNumber(contArgs));
    XtManageChild(ArticleContainer);

    /* article display text window */
    ArticleTextText = XmCreateScrolledText(ArticleContainer, "articleTextText",
					   articleTextArgs,
					   XtNumber(articleTextArgs));
    XtManageChild(ArticleTextText);
    ArticleText = ArticleTextText;
    
    /* all groups list window */
    ArticleTextList = XmCreateScrolledList(ArticleContainer, "articleTextList",
					   aListArgs, XtNumber(aListArgs));
    XtManageChild(ArticleTextList);

    BottomInfoLine = XmCreateLabel(Frame, "textinfo",
				   genericArgs, XtNumber(genericArgs));
    XtManageChild(BottomInfoLine);

    XtManageChild(XmCreateSeparator(Frame, "separator",
				    sepArgs, XtNumber(sepArgs)));

    BottomButtonBox = XmCreateRowColumn(Frame, "textbuttons",
					boxArgs, XtNumber(boxArgs));
    XtManageChild(BottomButtonBox);
#endif

    /* initialize the news system, read the newsrc file */
    initializeNews();
    XRNState |= XRN_NEWS_UP;

    /* set up the text window, mode buttons, and question */
    determineMode();

    /* create the icon */
    xmIconCreate();

#ifdef TITLEBAR
    XtSetArg(fontArgs[0], XtNheight, (XtPointer) &height);
    XtGetValues(titlebar, fontArgs, XtNumber(fontArgs));
#ifndef MOTIF
    XawPanedSetMinMax(titlebar, (int) height, (int) height);
#else
    {
      Arg args[2];

      XtSetArg(args[0], XmNpaneMaximum, height);
      XtSetArg(args[1], XmNpaneMinimum, height);
      XtSetValues(titlebar, args, 2);
    }
#endif
#endif    

    XtSetArg(fontArgs[0], XtNheight, &height);
    XtGetValues(TopInfoLine, fontArgs, XtNumber(fontArgs));
#ifndef MOTIF
    XawPanedSetMinMax(TopInfoLine, (int) height, (int) height);
#else
    {
      Arg args[2];

      XtSetArg(args[0], XmNpaneMaximum, height);
      XtSetArg(args[1], XmNpaneMinimum, height);
      XtSetValues(TopInfoLine, args, 2);
    }
#endif
    
    XtSetArg(fontArgs[0], XtNheight, &height);
    XtGetValues(BottomInfoLine, fontArgs, XtNumber(fontArgs));
#ifndef MOTIF
    XawPanedSetMinMax(BottomInfoLine, (int) height, (int) height);
    XtSetKeyboardFocus(Frame, ArticleText);	/* mikey */
#else
    {
      Arg args[2];

      XtSetArg(args[0], XmNpaneMaximum, height);
      XtSetArg(args[1], XmNpaneMinimum, height);
      XtSetValues(BottomInfoLine, args, 2);
    }
#endif
    
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

#if defined(MOTIF) && defined(EDITRES) && XtSpecificationRelease == 5
    /* Enable editres interaction (needs X11R5 Xmu; Xaw already has this) */
    {
      extern void _XEditResCheckMessages();

      XtAddEventHandler(TopLevel, (EventMask)0, True,
			_XEditResCheckMessages, (XtPointer) 0);
    }
#endif
    
    XtRealizeWidget(TopLevel);

#ifndef MOTIF
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
#else
#if XmVersion >= 1001
    XmProcessTraversal(ArticleText, XmTRAVERSE_CURRENT);
#else
    XtSetKeyboardFocus(Frame, ArticleText);	/* mikey */
#endif
#endif
    
    unbusyCursor();
    addTimeOut();
#if XtSpecificationRelease > 5
    XtAddCallback(TopLevel, XtNsaveCallback, saveNewsrcCB, NULL);
    XtAddCallback(TopLevel, XtNdieCallback, ehDieCB, NULL);
#endif /* X11R6 or greater */
    XRNState |= XRN_X_UP;
    if (app_resources.version == 0) {
	mesgPane(XRN_SERIOUS, NO_APP_DEFAULTS_MSG,
#ifdef MOTIF
		 "XRnMotif", "XRnMotif"
#else
		 "XRn", "XRn"
#endif
	    );
    } else if (strcmp(app_resources.version, XRN_VERSION) != 0) {
	mesgPane(XRN_SERIOUS, NO_APP_DEFAULTS_MSG,
#ifdef MOTIF
		 "XRnMotif", "XRnMotif"
#else
		 "XRn", "XRn"
#endif
	    );
	mesgPane(XRN_SERIOUS | XRN_APPEND, VERSIONS_MSG, app_resources.version,
		 XRN_VERSION);
    }

    xmSetIconAndName((unreadNews() ? UnreadIcon : ReadIcon));

    XtAppAddInput(XtWidgetToApplicationContext(TopLevel),
                inchannel, (XtPointer) XtInputReadMask, processMessage, (XtPointer) 0);

    XtAppMainLoop(XtWidgetToApplicationContext(TopLevel));
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

