/*	SCCSID @(#)readdir.c	1.3 05/12/16	*/

#include	"global.h"

#if	BSD4 == 0 && NDIRLIB == 0

#include "ndir.h"

/*
 * read an old style directory entry and present it as a new one
 */

#define	ODIRSIZ	14

struct	olddirect {
	ino_t	od_ino;
	char	od_name[ODIRSIZ];
};

/*
 * get next entry in a directory.
 */

struct direct *
readdir(dirp)
	register DIR *dirp;
{
	register struct olddirect *dp;

	for (;;) {
		if (dirp->dd_loc == 0) {
			dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, 
			    DIRBLKSIZ);
			if (dirp->dd_size <= 0)
				return NULL;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct olddirect *)(dirp->dd_buf + dirp->dd_loc);
		dirp->dd_loc += sizeof(struct olddirect);
		if (dp->od_ino == 0)
			continue;
		dirp->dd_dir.d_ino = dp->od_ino;
		(void)strncpy(dirp->dd_dir.d_name, dp->od_name, ODIRSIZ);
		dirp->dd_dir.d_name[ODIRSIZ] = '\0'; /* insure null termination */
		return (&dirp->dd_dir);
	}
}
#else	/* BSD4 == 0 && NDIRLIB == 0 */

void
__null3(){}

#endif	/* BSD4 == 0 && NDIRLIB == 0 */
