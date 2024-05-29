/*	SCCSID @(#)telldir.c	1.3 05/12/16	*/

#include	"global.h"

#if	BSD4 == 0 && NDIRLIB == 0

#include "ndir.h"

/*
 * return a pointer into a directory
 */

long
telldir(dirp)
	DIR *dirp;
{
	return (lseek(dirp->dd_fd, 0L, 1) - dirp->dd_size + dirp->dd_loc);
}
#else

void
__null5(){}

#endif
