/*	SCCSID @(#)seekdir.c	1.3 05/12/16	*/

#include	"global.h"

#if	BSD4 == 0 && NDIRLIB == 0

#include "ndir.h"

/*
 * seek to an entry in a directory.
 * Only values returned by ``telldir'' should be passed to seekdir.
 */

void
seekdir(dirp, loc)
	register DIR *dirp;
	long loc;
{
	long curloc, base, offset;
	struct direct *dp;

	curloc = telldir(dirp);
	if (loc == curloc)
		return;
	base = loc & ~(DIRBLKSIZ - 1);
	offset = loc & (DIRBLKSIZ - 1);
	if (dirp->dd_loc != 0 && (curloc & ~(DIRBLKSIZ - 1)) == base) {
		dirp->dd_loc = offset;
		return;
	}
	(void)lseek(dirp->dd_fd, base, 0);
	dirp->dd_loc = 0;
	while (dirp->dd_loc < offset) {
		dp = readdir(dirp);
		if (dp == NULL)
			return;
	}
}
#else	/* BSD4 == 0 && NDIRLIB == 0 */

void
__null4(){}

#endif	/* BSD4 == 0 && NDIRLIB == 0 */
