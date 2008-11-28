#include "config.h"
#include "utils.h"
#include <stdio.h>

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
    char *rs = (*s1) ? 0 : (char *) s2;

    while (*s1)
    {   if (strncmp(s2,s1,n) == 0L)
	{   rs = (char *) s1;
	    break;
	}
	++s1;
    }
    return(rs);
}
#endif

