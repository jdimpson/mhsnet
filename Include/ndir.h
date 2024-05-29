/* Copyright (c) 1982 Regents of the University of California */

/* @(#)ndir.h 4.4 3/30/82 */
/* MHSnet version -- @(#)ndir.h	1.5 94/04/30 */

#ifndef	_TYPES_
#include	<sys/types.h>
#endif

#if	NDIRLIB >= 1 || (BSD4 == 1 && BSD4V == 'a') || (SYSTEM == 5 && SYSVREL >= 3)
#if	NDIRLIB == 2 || (SYSTEM == 5 && SYSVREL >= 3)
#include	<dirent.h>
#define	direct	dirent
#else	/* NDIRLIB == 2 || (SYSTEM == 5 && SYSVREL >= 3) */
#include	"/usr/include/ndir.h"
#endif	/* NDIRLIB == 2 || (SYSTEM == 5 && SYSVREL >= 3) */
#ifndef	MAXNAMLEN
#define	MAXNAMLEN	255
#endif	/* MAXNAMLEN */
#else	/* NDIRLIB >= 1 || (BSD4 == 1 && BSD4V == 'a') || (SYSTEM == 5 && SYSVREL >= 3) */
#if	BSD4 >= 2 || (BSD4 == 1 && BSD4V > 'a')
#include	<sys/dir.h>
#else	/* BSD4 >= 2 || (BSD4 == 1 && BSD4V > 'a') */

#ifndef	DIRSIZ
#define		DIRSIZ	14
#endif

/*
 * This sets the "page size" for directories.
 */
#define DIRBLKSIZ FILEBUFFERSIZE

/*
 * This limits the directory name length.
 */
#define MAXNAMLEN DIRSIZ

struct	direct {
	ino_t	d_ino;
	char	d_name[MAXNAMLEN + 1];
	/* typically shorter */
};

typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	struct direct dd_dir;
	char	dd_buf[DIRBLKSIZ];
} DIR;

/*
 * functions defined on directories
 */
extern DIR *opendir();
extern struct direct *readdir();
extern long telldir();
extern void seekdir();
extern void closedir();

#endif	/* BSD4 >= 2 || (BSD4 == 1 && BSD4V > 'a') */
#endif	/* NDIRLIB >= 1 || (BSD4 == 1 && BSD4V == 'a') || (SYSTEM == 5 && SYSVREL >= 3) */

/*
 * useful macros.
 */

#ifndef	QNX
#ifndef	rewinddir
#define rewinddir(dirp)	seekdir(dirp, 0L)
#endif	/* rewinddir */
#endif	/* QNX */

#ifndef	NULL
#define	NULL	0
#endif
