#ifndef _BUSYCURSOR_H_
#define _BUSYCURSOR_H_

#include <X11/Intrinsic.h>

#ifdef XRN
#include "utils.h"
#endif

#ifndef _ARGUMENTS
#ifdef __STDC__
#define _ARGUMENTS(a) a
#else
#define _ARGUMENTS(a) ()
#endif
#endif /* !_ARGUMENTS */

extern void	BusyCursor	_ARGUMENTS((Widget, Boolean));
extern void	UnbusyCursor	_ARGUMENTS((Widget, Boolean));

#endif /* _BUSYCURSOR_H_ */
