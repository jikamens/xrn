#ifndef ERROR_HANDLERS_H
#define ERROR_HANDLERS_H

/*
 * $Id: error_hnds.h,v 1.8 1996-03-29 06:00:51 jik Exp $
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
 * error_handlers.h: routines for error/signal handling
 *
 */

extern void ehErrorExitXRN _ARGUMENTS((char *message));
extern void ehSignalExitXRN _ARGUMENTS((char *message));
extern void ehCleanExitXRN _ARGUMENTS((void));
extern void ehNoUpdateExitXRN _ARGUMENTS((void));
extern int ehErrorRetryXRN _ARGUMENTS((char *message, /* Boolean */ int save));

#if XtSpecificationRelease > 5
extern void saveNewsrcCB _ARGUMENTS((Widget w, XtPointer client_d,
				     XtPointer call_d));
extern void ehDieCB _ARGUMENTS((Widget w, XtPointer client_d,
				XtPointer call_d));
#endif

/* install the X and Xtoolkit error handlers */
extern void ehInstallErrorHandlers _ARGUMENTS((void));

/* install the signal handlers */
extern void ehInstallSignalHandlers _ARGUMENTS((void));

#if !defined(__NetBSD__) && !defined(__FreeBSD__)
extern int errno, sys_nerr;
extern char *sys_errlist[];
#endif

#define errmsg(a) ((a < sys_nerr) ? sys_errlist[a] : "unknown error")
     
#endif /* ERROR_HANDLERS_H */
