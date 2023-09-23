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

#ifndef ALLMODE_H
#define ALLMODE_H

#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"
#include "utils.h"

BUTDECL(allQuit);
BUTDECL(allSub);
BUTDECL(allNext);
BUTDECL(allPrev);
BUTDECL(allScroll);
BUTDECL(allScrollBack);
BUTDECL(allSearch);
BUTDECL(allLimit);
BUTDECL(allFirst);
BUTDECL(allLast);
BUTDECL(allAfter);
BUTDECL(allUnsub);
BUTDECL(allIgnore);
BUTDECL(allGoto);
BUTDECL(allSelect);
BUTDECL(allMove);
BUTDECL(allToggle);
BUTDECL(allPost);
BUTDECL(allPostAndMail);
BUTDECL(allMail);

extern XtActionsRec AllActions[];
extern int AllActionsCount;

extern void redrawAllWidget _ARGUMENTS((void));
extern void updateAllWidget _ARGUMENTS((String, long, long));
extern void switchToAllMode _ARGUMENTS((void));

extern void displayAllWidgets _ARGUMENTS((void));
extern void hideAllWidgets _ARGUMENTS((void));

extern void allDoTheRightThing _ARGUMENTS((Widget, XEvent *, String *,
					   Cardinal *));

#endif /* ALLMODE_H */
