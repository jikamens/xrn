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
