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
BUTDECL(artKillSession);
BUTDECL(artKillLocal);
BUTDECL(artKillGlobal);
BUTDECL(artKillAuthor);
BUTDECL(artSubSearch);
BUTDECL(artContinue);
BUTDECL(artPost);
BUTDECL(artPostAndMail);
BUTDECL(artMail);
BUTDECL(artExit);
BUTDECL(artCheckPoint);
BUTDECL(artGripe);
BUTDECL(artListOld);

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
