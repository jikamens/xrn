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
