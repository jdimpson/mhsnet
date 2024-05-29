/*	SCCSID @(#)opendir.c	1.4 05/12/16	*/

#define	STAT_CALL

#include	"global.h"

#if	BSD4 == 0 && NDIRLIB == 0

#include "ndir.h"

/*
 * open a directory.
 */

DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	struct stat sbuf;

	dirp = (DIR *)Malloc(sizeof(DIR));
	dirp->dd_fd = open(name, 0);
	if (dirp->dd_fd == SYSERROR) {
		free((char *)dirp);
		return NULL;
	}
	(void)fstat(dirp->dd_fd, &sbuf);
	if ((sbuf.st_mode & S_IFMT) != S_IFDIR) {
		(void)close(dirp->dd_fd);
		free((char *)dirp);
		return NULL;
	}
	dirp->dd_loc = 0;
	return dirp;
}
#else	/* BSD4 == 0 && NDIRLIB == 0 */

void
__null2(){}

#endif	/* BSD4 == 0 && NDIRLIB == 0 */
