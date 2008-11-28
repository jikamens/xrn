#ifndef ARTMODE_H
#define ARTMODE_H

#include <X11/Intrinsic.h>
#include "config.h"
#include "butdefs.h"
#include "utils.h"
#include "buttons.h"

BUTDECL(artQuit);
BUTDECL(artNext);
BUTDECL(artNextUnread);
BUTDECL(artPrev);
BUTDECL(artLast);
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

extern ButtonList ArtButtonList[];
extern int ArtButtonListCount;

extern ButtonList ArtSpecButtonList[];
extern int ArtSpecButtonListCount;

extern XtActionsRec ArtActions[];
extern int ArtActionsCount;

extern char *ArticleNewsGroupsString;

extern int switchToArticleMode _ARGUMENTS((void));

#endif /* ARTMODE_H */
