#ifndef _BUTTONBOX_H_
#define _BUTTONBOX_H_

#include <X11/Intrinsic.h>

#include "utils.h"

extern Widget	ButtonBoxCreate		_ARGUMENTS((String, Widget));
extern Widget	ButtonBoxAddButton	_ARGUMENTS((String, XtCallbackRec *,
						    Widget));
extern void	ButtonBoxDoneAdding	_ARGUMENTS((Widget));
extern void	ButtonBoxDestroy	_ARGUMENTS((Widget));

#endif /* _BUTTONBOX_H_ */
