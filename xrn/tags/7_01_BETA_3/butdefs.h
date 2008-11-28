#ifndef BUTDEFS_H
#define BUTDEFS_H

/*
 * $Header: /d/src/cvsroot/xrn/butdefs.h,v 1.4 1994-10-16 15:26:22 jik Exp $
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

#undef lint
#ifdef lint
#define BUTTON(nm,lbl)
#else
#ifdef MOTIF
#define BUTTON_CALL_DATA_T XmPushButtonCallbackStruct*
#define BUTTON_EVENT (call_data->event)
#else
#define BUTTON_CALL_DATA_T XtPointer
#define BUTTON_EVENT ((XEvent*)NULL)
#endif
#if defined(__STDC__) && !defined(UNIXCPP)
#define BUTTON(nm,lbl)				\
static void nm##Function _ARGUMENTS((Widget,XEvent *, String *, Cardinal *));\
static void nm##Core(Widget widget,XEvent *event,String *string, \
		     Cardinal *count)\
{						\
    if (inCommand) {				\
	return;					\
    }						\
    inCommand = 1;				\
    removeTimeOut();				\
    busyCursor();				\
    nm##Function(widget, event, string, count);	\
    unbusyCursor();				\
    addTimeOut();				\
    inCommand = 0;				\
    return;					\
}						\
/*ARGSUSED*/                                    \
static void nm##Action(Widget widget, XEvent *event, String *string, \
		       Cardinal *count) \
{						\
    nm##Core(widget, event, string, count);	\
    return;					\
}						\
/*ARGSUSED*/                                    \
static void nm##Button(Widget widget, XtPointer client_data, \
		       BUTTON_CALL_DATA_T call_data)	\
{						\
    nm##Core(widget, BUTTON_EVENT, 0, 0);	\
    return;					\
}						\
static XtCallbackRec nm##Callbacks[] = {	\
    {(XtCallbackProc) nm##Button, NULL},	\
    {NULL, NULL}				\
};						\
static Arg nm##Args[] = {			\
    {XtNname, (XtArgVal) #nm},			\
    {MyNcallback, (XtArgVal) nm##Callbacks}	\
}
#else
#define BUTTON(nm,lbl)				\
static void nm/**/Function _ARGUMENTS((Widget,XEvent *, String *, \
				       Cardinal *));\
static void nm/**/Core(widget, event, string, count) \
Widget widget;					\
XEvent *event;					\
String *string;					\
Cardinal *count;				\
{						\
    if (inCommand) {				\
	return;					\
    }						\
    inCommand = 1;				\
    removeTimeOut();				\
    busyCursor();				\
    nm/**/Function(widget, event, string, count); \
    unbusyCursor();				\
    addTimeOut();				\
    inCommand = 0;				\
    return;					\
}						\
/*ARGSUSED*/					\
static void nm/**/Action(widget, event, string, count) \
Widget widget;					\
XEvent *event;					\
String *string;					\
Cardinal *count;				\
{						\
    nm/**/Core(widget, event, string, count);	\
    return;					\
}						\
/*ARGSUSED*/					\
static void nm/**/Button(widget, client_data, call_data)	\
Widget widget;					\
XtPointer client_data;				\
BUTTON_CALL_DATA_T call_data;			\
{						\
    nm/**/Core(widget, BUTTON_EVENT, 0, 0);	\
    return;					\
}						\
static XtCallbackRec nm/**/Callbacks[] = {	\
    {(XtCallbackProc) nm/**/Button, NULL},	\
    {NULL, NULL}				\
};						\
static Arg nm/**/Args[] = {			\
    {XtNname, (XtArgVal) "nm"},			\
/*    {XtNlabel, (XtArgVal) "lbl"}, */		\
    {MyNcallback, (XtArgVal) nm/**/Callbacks}	\
}
#endif
#endif

#endif /* BUTDEFS_H */
