#ifndef ALLMODE_H
#define ALLMODE_H

#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"
#include "utils.h"

BUTDECL(allQuit);
BUTDECL(allSub);
BUTDECL(allFirst);
BUTDECL(allLast);
BUTDECL(allAfter);
BUTDECL(allUnsub);
BUTDECL(allGoto);
BUTDECL(allSelect);
BUTDECL(allMove);
BUTDECL(allToggle);
BUTDECL(allScroll);
BUTDECL(allScrollBack);
BUTDECL(allPost);

extern XtActionsRec AllActions[];
extern int AllActionsCount;

extern ButtonList AllButtonList[];
extern int AllButtonListCount;

extern void redrawAllWidget _ARGUMENTS((void));
extern void updateAllWidget _ARGUMENTS((String, long, long));
extern void switchToAllMode _ARGUMENTS((void));

#endif /* ALLMODE_H */
