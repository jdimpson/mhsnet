/*	SCCSID @(#)closedir.c	1.4 05/12/16	*/

#include	"global.h"

#if	BSD4 == 0 && NDIRLIB == 0

#include "ndir.h"

/*
 * close a directory.
 */

void
closedir(dirp)
	register DIR *dirp;
{
	(void)close(dirp->dd_fd);
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	free((char *)dirp);
}
#else

void
__null1(){}

#endif
