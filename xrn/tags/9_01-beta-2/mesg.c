#if !defined(lint) && !defined(SABER) && !defined(GCC_WALL)
static char XRNrcsid[] = "$Id: mesg.c,v 1.28 1998-01-28 21:18:29 jik Exp $";
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
 * mesg.c: message box
 */

#include "copyright.h"
#include "config.h"
#include "utils.h"
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>

#include "xthelper.h"
#include "resources.h"
#include "xrn.h"
#include "mesg.h"
#include "buttons.h"
#include "xmisc.h"
#include "butdefs.h"
#ifdef XRN_USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "ngMode.h"
#include "Text.h"
#include "ButtonBox.h"
#include "InfoLine.h"
#include "InfoDialog.h"
#include "mesg_strings.h"

char error_buffer[2048];
static char *MesgString = 0;

#ifndef XawFmt8Bit
#define XawFmt8Bit FMT8BIT
#endif

#define MESG_SIZE 4096
/*
 * If you have a window larger that 512 characters across, or there is
 * an info message to be displayed that is longer than 512 characters,
 * then someone should be shot!
 */
static char InfoString[512]; 


int newMesgPaneName()
{
    static int number = 0;

    number++;
    if (! number)
	number++;

    return number;
}


/*ARGSUSED*/
/*VARARGS2*/
#ifdef XRN_USE_STDARG
void
mesgPane(int type, int name, char *fmtString, ...)
#else
void
mesgPane(type, name, fmtString, va_alist)
int type, name;		/* XRN_INFO, XRN_WARNING, XRN_SERIOUS */
char *fmtString;
va_dcl
#endif /* XRN_USE_STDARG */
/*
 * brings up a new vertical pane, not moded
 *
 * the pane consists of 3 parts: title bar, scrollable text window,
 * button box
 */
{
    va_list args;
    static int last_name = 0;
    time_t tm;
    char *time_str;
    char addBuff[MESG_SIZE];
    static Boolean intro_displayed = False;
    char *separator = "\n--------\n";

    if (name && last_name && (name == last_name))
	type |= XRN_APPEND;
    last_name = name;

#ifdef XRN_USE_STDARG
    va_start(args, fmtString);
#else    
    va_start(args);
#endif

    tm = time(0);

    if (! (XRNState & XRN_X_UP)) {
	(void) fprintf(stderr, "%-24.24s: ", ctime(&tm));
	(void) vfprintf(stderr, fmtString, args);
	(void) fprintf(stderr, "\n");
	return;
    }

    if ((type & XRN_INFO) && (app_resources.info == False)) {
	(void) vsprintf(InfoString, fmtString, args);
	infoNow(InfoString);
	return;
    }

    time_str = ctime(&tm);
    time_str += 11; /* Skip over the day and date */
    time_str[8] = '\0'; /* We only want the time, not the year and the newline */

    InfoDialogCreate(TopLevel);

    if (! (MesgLength() || MesgString)) {
	(void) sprintf(addBuff, "%s: ", time_str);
	if (! intro_displayed) {
	  intro_displayed = True;
	  (void) sprintf(&addBuff[strlen(addBuff)], "%s%s%s: ",
			 MESG_PANE_DISMISS_MSG, separator, time_str);
	}
    }
    else if (type & XRN_SAME_LINE) {
	*addBuff = '\0';
    }
    else if (type & XRN_APPEND) {
	(void) sprintf(addBuff, "\n%8s  ", "");
    }
    else {
	(void) sprintf(addBuff, "%s%s: ", separator, time_str);
    }

    (void) vsprintf(&addBuff[strlen(addBuff)], fmtString, args);

    displayMesgString(addBuff);
    
    return;
}

/*
 * put an informational 'msg' into the top information label
 *
 * If 'now' is true, then process as many X events as possible to
 * force the message to be displayed immediately.
 */
void _info(
	   _ANSIDECL(char *,	msg),
	   _ANSIDECL(Boolean,	now)
	   )
     _KNRDECL(char *,	msg)
     _KNRDECL(Boolean,	now)
{
    static char label[LABEL_SIZE] = "";
    static Widget info_widget = 0;
    static Boolean in_info = False;

    if (! in_info) {
	in_info = True;

	if (XRNState & XRN_X_UP) {
	    if ((info_widget != TopInfoLine) || strcmp(msg, label)) {
		(void)strncpy(label, msg, sizeof(label) - 1);
		label[sizeof(label) - 1] = '\0';
		info_widget = TopInfoLine;
		if (! TopInfoLine)
		    return;
		InfoLineSet(TopInfoLine, msg);

		if (now) {
		    xthHandlePendingExposeEvents();
		}
	    }
	}
	else {
	    (void) fprintf(stderr, "XRN: %s\n", msg);
	}

	in_info = False;
    }

    return;
}
