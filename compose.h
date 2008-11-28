#ifndef COMPOSE_H
#define COMPOSE_H

#include "butdefs.h"

/*
 * $Id: compose.h,v 1.7 1995-02-20 21:30:06 jik Exp $
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
 * compose.h: functions for composing and sending messages
 */

extern void post _ARGUMENTS((int ingroupp));
extern void followup _ARGUMENTS((void));
extern void reply _ARGUMENTS((void));
extern void followup_and_reply _ARGUMENTS((void));
extern void cancelArticle _ARGUMENTS((void));
extern Boolean canCancelArticle _ARGUMENTS((void));
extern void gripe _ARGUMENTS((void));
extern void forward _ARGUMENTS((void));
extern void processMessage _ARGUMENTS((XtPointer, int *, XtInputId *));
extern char *getUser _ARGUMENTS((void));

BUTDECL(compAbort);
BUTDECL(compSave);
BUTDECL(compSend);
BUTDECL(compIncludeArticle);
BUTDECL(compIncludeFile);

#endif /* COMPOSE_H */
