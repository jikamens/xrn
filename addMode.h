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

#ifndef ADDMODE_H
#define ADDMODE_H

#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"

BUTDECL(addQuit);
BUTDECL(addIgnoreRest);
BUTDECL(addFirst);
BUTDECL(addLast);
BUTDECL(addAfter);
BUTDECL(addUnsub);
BUTDECL(addIgnore);

extern XtActionsRec AddActions[];
extern int AddActionsCount;

extern void redrawAddTextWidget _ARGUMENTS((String, long));
extern void switchToAddMode _ARGUMENTS((String));

extern void displayAddWidgets _ARGUMENTS((void));
extern void hideAddWidgets _ARGUMENTS(());

#endif /* ADDMODE_H */
