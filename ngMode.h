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
BUTDECL(ngPrevGroup);
BUTDECL(ngSelect);
BUTDECL(ngMove);
BUTDECL(ngCheckPoint);
BUTDECL(ngPost);
BUTDECL(ngGripe);
BUTDECL(ngScroll);
BUTDECL(ngScrollBack);

extern XtActionsRec NgActions[];
extern int NgActionsCount;

extern ButtonList NgButtonList[];
extern int NgButtonListCount;

extern char CurrentIndexGroup[];
extern char LastGroup[];

extern int NewsgroupDisplayMode;

#define NG_ENTRY_JUMP 0
#define NG_ENTRY_GOTO 1

extern int NewsgroupEntryMode;

extern void switchToNewsgroupMode _ARGUMENTS((/* Boolean */ int));
extern void redrawNewsgroupTextWidget _ARGUMENTS((String, /* Boolean */ int,
				                  /* Boolean */ int));
extern void updateNewsgroupMode _ARGUMENTS((/* Boolean */ int, /* Boolean */ int));
extern void doPrefetch _ARGUMENTS((Widget, XEvent *, String *, Cardinal *));
extern void addTimeOut _ARGUMENTS((void));
extern void removeTimeOut _ARGUMENTS((void));

#endif /* NGMODE_H */
