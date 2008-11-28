#ifndef ERROR_HANDLERS_H
#define ERROR_HANDLERS_H

/*
 * $Header: /d/src/cvsroot/xrn/error_hnds.h,v 1.3 1994-10-11 14:23:39 jik Exp $
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
extern void ehCleanExitXRN _ARGUMENTS(());
extern void ehNoUpdateExitXRN _ARGUMENTS(());
extern int ehErrorRetryXRN _ARGUMENTS((char *message, /* Boolean */ int save));

/* install the X and Xtoolkit error handlers */
extern void ehInstallErrorHandlers _ARGUMENTS(());

/* install the signal handlers */
extern void ehInstallSignalHandlers _ARGUMENTS(());

extern int errno, sys_nerr;
extern char *sys_errlist[];

#define errmsg(a) ((a < sys_nerr) ? sys_errlist[a] : "unknown error")
     
#endif /* ERROR_HANDLERS_H */
