
#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: error_hnds.c,v 1.22 2005-12-01 08:47:56 jik Exp $";
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
 * error_handlers.c: routines for error/signal handling
 *
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <errno.h>
#include "dialogs.h"
#include "xmisc.h"
#include "news.h"
#include "xthelper.h"
#include "internals.h"
#include "xrn.h"
#include "mesg.h"
#include "error_hnds.h"
#include "resources.h"
#include "newsrcfile.h"
#include "file_cache.h"
#include "mesg_strings.h"


/*
 * error handlers - major purpose is to close down 'rn' cleanly
 */

static int xrnXIOError _ARGUMENTS((Display *));

/*ARGSUSED*/
static int xrnXIOError(display)
    Display *display;
{
    XRNState &= ~XRN_X_UP;
    sprintf(error_buffer, "XIO Error: %s", errmsg(errno));
    ehErrorExitXRN(error_buffer);
    /* NOTREACHED */
    return(0);
}

static int xrnXError _ARGUMENTS((Display *, XErrorEvent *));

/*ARGSUSED*/
static int xrnXError(display, event)
    Display *display;
    XErrorEvent *event;
{
    char buffer[1024];

#ifdef ERRORBUG
	if (event->request_code == 4) 		/* destroy window errors */
	return;					/* Bug workaround */
#endif

    XGetErrorText(display, event->error_code, buffer, sizeof(buffer));
    (void) fprintf(stderr, "xrn: X Error: %s\n", buffer);
    (void) fprintf(stderr, "    serial number: %ld\n", event->serial);
    (void) fprintf(stderr, "    error code:  %d\n", (int) event->error_code);
    (void) fprintf(stderr, "    request code:  %d\n", (int) event->request_code);
    (void) fprintf(stderr, "    minor code:  %d\n", (int) event->minor_code);
    (void) fprintf(stderr, "    resource id: %d\n", (int) event->resourceid);
    XRNState &= ~XRN_X_UP;
    ehErrorExitXRN("X Error");
    /*NOTREACHED*/
    return(0);
}

static int xrnXtError _ARGUMENTS((String));

static int xrnXtError(errorMessage)
    String errorMessage;
{
#define XTERRORINTRO "X Toolkit Error: "
    char buffer[80];
    strcpy(buffer, XTERRORINTRO);
    strncat(buffer, errorMessage, sizeof(buffer) - sizeof(XTERRORINTRO));
    (void) fprintf(stderr, "xrn: %s\n", buffer);
    XRNState &= ~XRN_X_UP;
    ehErrorExitXRN(buffer);
#undef XTERRORINTRO
    /*NOTREACHED*/
    return(0);
}

void ehInstallErrorHandlers()
{
    XtAppSetErrorHandler(TopContext, (XtErrorHandler) xrnXtError);
    XSetErrorHandler(xrnXError);
    XSetIOErrorHandler(xrnXIOError);
    return;
}


static RETSIGTYPE sig_catcher _ARGUMENTS((int));

static RETSIGTYPE sig_catcher(signo)
    int signo;
{
    char buffer[80];
    
    /* allow HUP's and INT's to do a clean exit */
    if ((signo == SIGHUP) || (signo == SIGINT)) {
	ehCleanExitXRN();
    }

    (void) sprintf(buffer, "Caught signal (%d), cleaned up .newsrc and removed temp files", signo);

    (void) signal(signo, SIG_DFL);
    ehSignalExitXRN(buffer);
    (void) kill(getpid(), signo);
    /*NOTREACHED*/
#ifdef SIGFUNC_RETURNS
    return(0);
#endif
}

void ehInstallSignalHandlers()
{
    int i;
#ifdef SIGFUNC_RETURNS
    int (*oldcatcher)(int);
#else
    void (*oldcatcher)(int);
#endif

    for (i = 1; i <= SIGTERM; i++) {
	switch (i) {
#ifdef SIGSTOP
	case SIGSTOP:
#endif
#ifdef SIGTSTP
	case SIGTSTP:
#endif
#ifdef SIGCONT
	case SIGCONT:
#endif
	    break;

	case SIGPIPE:
	    (void) signal(i, SIG_IGN);
	    break;

	default:
	    if (! app_resources.dumpCore) {
		oldcatcher = signal(i, sig_catcher);
		if (oldcatcher == SIG_IGN) {
		    (void) signal(i, SIG_IGN);
		}
	    }
	    break;
	}
    }
    return;
}

static int retry;
static int die;

static void myAbort()
{
    die = 1;
    return;
}    


static void myExit()
{
    exit(-1);
}

static void Retry()
{
    retry = 1;
    return;
}

static void deathNotifier _ARGUMENTS((char *));

static void deathNotifier(message)
    char *message;
{
    XEvent ev;

    static struct DialogArg args[] = {
	{"abort", myAbort, (XtPointer) -1},
	{"exit", myExit, (XtPointer) -1},
    };

    die = 0;

    cancelPrefetch();

    if (! (XRNState & XRN_X_UP)) {
	(void) fprintf(stderr, "xrn: %s\n", message);
	return;
    }

    /* XXX unmap icon */
    XtMapWidget(TopLevel);
    PopUpDialog(CreateDialog(TopLevel, message, DIALOG_NOTEXT, args, XtNumber(args)));

    while (!die) {
	XtAppNextEvent(TopContext, &ev);
	MyDispatchEvent(&ev);
    }

    return;
}

static int retryNotifier _ARGUMENTS((char *));

static int retryNotifier(message)
    char *message;
{
    XEvent ev;
    Widget dialog;

    static struct DialogArg args[] = {
	{"exit", myAbort, (XtPointer) -1},
	{"retry", Retry, (XtPointer) -1},
    };

    die = retry = 0;

    suspendPrefetch();

    if (! (XRNState & XRN_X_UP)) {
	(void) fprintf(stderr, "xrn: %s\n", message);
	return 0;
    }

    /* XXX unmap icon */
    XtMapWidget(TopLevel);
    dialog = CreateDialog(TopLevel, message, DIALOG_NOTEXT, args,
			  XtNumber(args));
    PopUpDialog(dialog);

    while (!retry && !die) {
	XtAppNextEvent(TopContext, &ev);
	MyDispatchEvent(&ev);
    }

    PopDownDialog(dialog);

    if (retry)
      resetPrefetch();

    return retry;
}

#define XRN_NORMAL_EXIT_BUT_NO_UPDATE 2
#define XRN_NORMAL_EXIT 1
#define XRN_ERROR_EXIT 0
#define XRN_SIGNAL_EXIT -1
      
static void exitXRN _ARGUMENTS((int));

static void exitXRN(status)
    int status;
{
    static int beenHere = 0;
    int do_exit = 0;

    /*
     * immediate exit, exitXRN was called as a result of something in
     * itself
     */ 
    if (beenHere) {
      do_exit++;
    }
    else {
      beenHere++;

      if ((XRNState & XRN_NEWS_UP) == XRN_NEWS_UP) {
	/* XXX is this really needed?  does free files... */
	releaseNewsgroupResources(CurrentGroup);

	cancelPrefetch();
	cancelRescanBackground();
	(void) file_cache_destroy(FileCache);
	FileCache = 0;
	if (status != XRN_NORMAL_EXIT_BUT_NO_UPDATE) {
	  if (status == XRN_NORMAL_EXIT) {
	    while (!updatenewsrc()) {
	      ehErrorRetryXRN("Cannot update the newsrc file", True);
	    }
	  } else {
	    if (!updatenewsrc()) {
	      fprintf(stderr, "xrn: .newsrc file update failed\n");
	    }
	  }
	}
      }
    }

    /* clean up the lock */
    removeLock();

    /* close down the NNTP server */
    close_server();

    if (do_exit)
      exit(-1);
}


void ehNoUpdateExitXRN()
{
    exitXRN(XRN_NORMAL_EXIT_BUT_NO_UPDATE);
    exit(0);
}

void ehCleanExitXRN()
{
    exitXRN(XRN_NORMAL_EXIT);
    exit(0);
}

void ehErrorExitXRN(message)
    char *message;
{
    exitXRN(XRN_ERROR_EXIT);
    if (message)
	deathNotifier(message);
    exit(-1);
}

int ehErrorRetryXRN(
		    _ANSIDECL(char *,	message),
		    _ANSIDECL(Boolean,	save)
		    )
     _KNRDECL(char *,	message)
     _KNRDECL(Boolean,	save)
{
    int retry;
     
    retry = retryNotifier(message);

    if (!retry && save) {
	exitXRN(XRN_ERROR_EXIT);
	exit(-1);
    }
    
    return retry;
}

void ehSignalExitXRN(message)
    char *message;
{
    exitXRN(XRN_SIGNAL_EXIT);
    deathNotifier(message);
    return;
}    


#if XtSpecificationRelease > 5
void
saveNewsrcCB(widget, client_data, call_data)
/*
 * SessionManager callback to write out an up to date copy of the .newsrc file
 *
 *   sets save_success False in the checkpoint token on failure.
 *
 */
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    if (!updatenewsrc()) {
	XtCheckpointToken cp = (XtCheckpointToken) call_data;
	cp->save_success = False;
    }
}


void
ehDieCB(widget, client_data, call_data)
/*
 * SessionManager callback to quit
 *
 *   tries one last time to save .newsrc if previous attempt failed.
 *
 */
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    XtCheckpointToken cp = (XtCheckpointToken) call_data;

    if ((cp && !cp->save_success) && !updatenewsrc()) { /* one last try */
	/* refer to ehErrorExitXRN */
	exitXRN(XRN_ERROR_EXIT);
	exit(-1);
    }
    
    ehNoUpdateExitXRN();
}
#endif
