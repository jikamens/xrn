#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: xrn.c,v 1.30 1997-05-21 00:32:18 jik Exp $";
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
#include <X11/Intrinsic.h> /* so we have Widget */

#include "news.h"
#include "xthelper.h"
#include "xmisc.h"
#include "resources.h"
#include "internals.h"
#include "error_hnds.h"
#include "buttons.h"
#include "mesg.h"
#include "xrn.h"
#include "patchlevel.h"
#include "compose.h"
#include "mesg_strings.h"
#include "InfoLine.h"
#include "Frame.h"
#include "ngMode.h"

#ifdef XFILESEARCHPATH
static void AddPathToSearchPath _ARGUMENTS((char *));
#endif

/* global variables that represent the widgets that are dynamically changed */

Widget TopLevel;
XtAppContext TopContext;
Widget TopInfoLine;      /* top button info line                      */
Widget BottomInfoLine;   /* bottom button info line                   */

int XRNState;            /* XRN status: news and x                    */

int inchannel, outchannel;

/*ARGSUSED*/
int main(argc, argv)
    int argc;
    char **argv;
{
    Widget frame;

    int sv[2];

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
       GetMainFrameSize(TopLevel, (app_resources.geometry));
    }
    
    /* create the pane and its widgets */

    frame = CreateMainFrame(TopLevel);

    TopInfoLine = InfoLineCreate("info", "", frame);
    BottomInfoLine = 0;
    
    createButtons();

    /* create the icon */
    xmIconCreate();

    XrnAddCloseCallbacks(TopLevel);

    /*
      Be sure that initializeNews() and determineMode() get called after
      the main window is already realized.  -- jik 11/13/94
      */
    
    XtRealizeWidget(TopLevel);
    XRNState |= XRN_X_UP;
    xthWaitForMapped(TopLevel, False);
#ifdef MOTIF
    XmUpdateDisplay(TopLevel);
#endif

    /* initialize the news system, read the newsrc file */
    initializeNews();
    XRNState |= XRN_NEWS_UP;

    /* set up the text window, mode buttons, and question */
    determineMode();

    xrnUnbusyCursor();
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

#if XtSpecificationRelease < 6
    MyMainLoop(TopContext);
#else
    XtAppMainLoop(TopContext);
#endif
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

#if XtSpecificationRelease < 6
static XEvent last_event;

XEvent *XtLastEventProcessed(display)
     Display *display;
{
  return &last_event;
}

void MyMainLoop(app)
     XtAppContext app;
{
  XEvent event;

  for (;;) {
    XtAppNextEvent(app, &event);
    MyDispatchEvent(&event);
  }
}

Boolean MyDispatchEvent(event)
     XEvent *event;
{
  last_event = *event;
  return XtDispatchEvent(event);
}
#endif /* XtSpecificationRelease < 6 */
