#ifndef _INFOLINE_H_
#define _INFOLINE_H_

#include <X11/Intrinsic.h>

#include "utils.h"

Widget	InfoLineCreate	_ARGUMENTS((String, String, Widget));
void	InfoLineSet	_ARGUMENTS((Widget, String));
void	InfoLineDestroy	_ARGUMENTS((Widget));

#endif /* _INFOLINE_H_ */
