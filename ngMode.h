/*
 * xrn - an X-based NNTP news reader
 *
 * Copyright (c) 1994-2023, Jonathan Kamens.
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

#ifndef NGMODE_H
#define NGMODE_H

#include <X11/Intrinsic.h>
#include "butdefs.h"
#include "buttons.h"

BUTDECL(ngExit);
BUTDECL(ngQuit);
BUTDECL(ngRead);
BUTDECL(ngNext);
BUTDECL(ngPrev);
BUTDECL(ngCatchUp);
BUTDECL(ngSubscribe);
BUTDECL(ngUnsub);
BUTDECL(ngGoto);
BUTDECL(ngListOld);
BUTDECL(ngAllGroups);
BUTDECL(ngRescan);
BUTDECL(ngGetList);
BUTDECL(ngPrevGroup);
BUTDECL(ngSelect);
BUTDECL(ngMove);
BUTDECL(ngCheckPoint);
BUTDECL(ngPost);
BUTDECL(ngPostAndMail);
BUTDECL(ngMail);
BUTDECL(ngGripe);
BUTDECL(ngScroll);
BUTDECL(ngScrollBack);

extern XtActionsRec NgActions[];
extern int NgActionsCount;

extern char *CurrentIndexGroup;
extern char *LastGroup;

extern int NewsgroupDisplayMode;

extern void switchToNewsgroupMode _ARGUMENTS((Boolean));
extern void redrawNewsgroupTextWidget _ARGUMENTS((String, Boolean));
extern void updateNewsgroupMode _ARGUMENTS((Boolean, Boolean));
extern void doPrefetch _ARGUMENTS((Widget, XEvent *, String *, Cardinal *));
extern void addTimeOut _ARGUMENTS((void));
extern void removeTimeOut _ARGUMENTS((void));

extern void displayNewsgroupWidgets _ARGUMENTS((void));
extern void hideNewsgroupWidgets _ARGUMENTS((void));
extern String getNewsgroupString _ARGUMENTS((void));

extern void ngDoTheRightThing _ARGUMENTS((Widget, XEvent *, String *,
					  Cardinal *));

#endif /* NGMODE_H */
