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

#ifndef ARTMODE_H
#define ARTMODE_H

#include <X11/Intrinsic.h>
#include "config.h"
#include "butdefs.h"
#include "utils.h"
#include "buttons.h"

BUTDECL(artQuit);
BUTDECL(artNextUnread);
BUTDECL(artNext);
BUTDECL(artPrev);
BUTDECL(artLast);
BUTDECL(artCurrent);
BUTDECL(artUp);
BUTDECL(artDown);
BUTDECL(artNextGroup);
BUTDECL(artGotoArticle);
BUTDECL(artCatchUp);
BUTDECL(artFedUp);
BUTDECL(artMarkRead);
BUTDECL(artMarkUnread);
BUTDECL(artSub);
BUTDECL(artUnsub);
BUTDECL(artScroll);
BUTDECL(artScrollBack);
BUTDECL(artScrollLine);
BUTDECL(artScrollBackLine);
BUTDECL(artScrollEnd);
BUTDECL(artScrollBeginning);
BUTDECL(artScrollIndex);
BUTDECL(artScrollIndexBack);
BUTDECL(artSubNext);
BUTDECL(artSubPrev);
BUTDECL(artThreadParent);
BUTDECL(artKillSubject);
BUTDECL(artKillAuthor);
BUTDECL(artKillThread);
BUTDECL(artKillSubthread);
BUTDECL(artSubSearch);
BUTDECL(artContinue);
BUTDECL(artPost);
BUTDECL(artPostAndMail);
BUTDECL(artMail);
BUTDECL(artExit);
BUTDECL(artCheckPoint);
BUTDECL(artGripe);
BUTDECL(artListOld);
BUTDECL(artResort);

BUTDECL(artSave);
BUTDECL(artReply);
BUTDECL(artForward);
BUTDECL(artFollowup);
BUTDECL(artFollowupAndReply);
BUTDECL(artCancel);
BUTDECL(artRot13);
#ifdef XLATE
BUTDECL(artXlate);
#endif /* XLATE */
BUTDECL(artHeader);
BUTDECL(artPrint);

extern XtActionsRec ArtActions[];
extern int ArtActionsCount;

extern int switchToArticleMode _ARGUMENTS((void));

extern void displayArticleWidgets _ARGUMENTS((void));
extern void hideArticleWidgets _ARGUMENTS((void));
extern void resetArticleNewsgroupsList _ARGUMENTS((void));

extern void artDoTheRightThing _ARGUMENTS((Widget, XEvent *, String *,
					   Cardinal *));

#endif /* ARTMODE_H */
