#include "config.h"
#include "utils.h"

#ifdef NEED_STRSTR
#include <X11/Xos.h>	/* for strlen declaration */

/*
===========================================================================
Marc Evans - Marc@Synergytics.COM           | Synergytics     (603)635-8876
WB1GRH     - WB1GRH@W2XO.#WPA.PA.USA.NOAM   | 21 Hinds Ln, Pelham, NH 03076
---------------------------------------------------------------------------
                      Unix and X Software Consultant
===========================================================================
*/

char * strstr(s1, s2)
    register char CONST *s1;
    register char CONST *s2;
{
    register int n = strlen(s2);

    if (! *s2)
	return (char *) s1;

    while (*s1) {
	if (! strncmp(s2,s1,n))
	    return (char *) s1;
	s1++;
    }
    return((char *) 0);
}
#endif

