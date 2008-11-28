#ifndef BUTDEFS_H
#define BUTDEFS_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "utils.h"
#include "xrn.h"
#include "xmisc.h"

/*
 * $Id: butdefs.h,v 1.6 1995-01-25 03:17:52 jik Exp $
 */

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
 * butdefs.h: define buttons
 *
 */

#define BUTTON_CALL_DATA_T XtPointer
#define BUTTON_EVENT ((XEvent*)NULL)

#define BUTDECL(nm)\
\
extern void CONCAT(nm,Function) _ARGUMENTS((Widget, XEvent *, String *,\
					    Cardinal *));\
extern void CONCAT(nm,Core) _ARGUMENTS((Widget, XEvent *, String *, Cardinal *));\
extern void CONCAT(nm,Action) _ARGUMENTS((Widget, XEvent *, String *, Cardinal *));\
extern void CONCAT(nm,Button) _ARGUMENTS((Widget, XtPointer, BUTTON_CALL_DATA_T));\
extern XtCallbackRec CONCAT(nm,Callbacks)[];\
extern Arg CONCAT(nm,Args)[]

#define BUTTON(nm,lbl)\
\
void CONCAT(nm,Core)(widget, event, string, count)\
    Widget widget;\
    XEvent *event;\
    String *string;\
    Cardinal *count;\
{\
    if (inCommand) {\
	return;\
    }\
    inCommand = 1;\
    removeTimeOut();\
    busyCursor();\
    CONCAT(nm,Function)(widget, event, string, count);\
    unbusyCursor();\
    addTimeOut();\
    inCommand = 0;\
    return;\
}\
\
void CONCAT(nm,Action)(widget, event, string, count)\
    Widget widget;\
    XEvent *event;\
    String *string;\
    Cardinal *count;\
{\
    CONCAT(nm,Core)(widget, event, string, count);\
    return;\
}\
\
void CONCAT(nm,Button)(widget, client_data, call_data)\
    Widget widget;\
    XtPointer client_data;\
    BUTTON_CALL_DATA_T call_data;\
{\
    CONCAT(nm,Core)(widget, BUTTON_EVENT, 0, 0);\
    return;\
}\
\
XtCallbackRec CONCAT(nm,Callbacks)[] = {\
    {(XtCallbackProc) CONCAT(nm,Button), NULL},\
    {NULL, NULL}\
};\
\
Arg CONCAT(nm,Args)[] = {\
    {XtNname, (XtArgVal) STRINGIFY(nm)},\
    {MyNcallback, (XtArgVal) CONCAT(nm,Callbacks)}\
}

#endif /* BUTDEFS_H */
