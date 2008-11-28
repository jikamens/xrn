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
