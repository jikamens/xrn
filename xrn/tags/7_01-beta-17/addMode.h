#ifndef ADDMODE_H
#define ADDMODE_H

#include <X11/Intrinsic.h>
#include "buttons.h"
#include "butdefs.h"

BUTDECL(addQuit);
BUTDECL(addFirst);
BUTDECL(addLast);
BUTDECL(addAfter);
BUTDECL(addUnsub);

extern XtActionsRec AddActions[];
extern int AddActionsCount;

extern ButtonList AddButtonList[];
extern int AddButtonListCount;

extern void redrawAddTextWidget _ARGUMENTS((String, long));
extern void switchToAddMode _ARGUMENTS((String));

#endif /* ADDMODE_H */
