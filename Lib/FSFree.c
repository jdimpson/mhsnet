/*
**	Copyright 2012 Piers Lauder
**
**	This file is part of MHSnet.
**
**	MHSnet is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	MHSnet is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with MHSnet.  If not, see <http://www.gnu.org/licenses/>.
*/


/*
**	Return true if enough space left on f/s for "file" to hold "space".
*/

#define	NEED_HZ		/* To pull in <sys/param.h> ! */
#define	STAT_CALL
#define	ERRNO

#include	"global.h"
#include	"debug.h"


#if	SYSTEM == 5 && SYS_STATFS != 1 && SYS_STATVFS != 1 && SYS_VFS != 1
#include	<ustat.h>

#ifndef	FS_BSIZE
#define	FS_BSIZE	512
#endif	/* FS_BSIZE */

int	FreeBSize	= FS_BSIZE;
#endif	/* SYSTEM == 5 && SYS_STATFS != 1 && SYS_STATVFS != 1 && SYS_VFS != 1 */

#if	SYS_STATVFS
#include	<sys/statvfs.h>
#endif	/* SYS_STATVFS */

#if	SYS_STATFS
#ifdef	__osf__
#include	<sys/mount.h>
int	FreeBSize	= 1024;
#else	/* __osf__ */
#include	<sys/statfs.h>
int	FreeBSize	= 512;
#endif	/* __osf__ */
#endif	/* SYS_STATFS */

#if	BSD4 == 2 || SYS_VFS == 1
#if	__bsdi__ == 1 || __NetBSD__ == 1 || __APPLE__ == 1
#include	<sys/mount.h>
int	FreeBSize	= 1024;
#else	/* __bsdi__ == 1 || __NetBSD__ == 1 */
#include	<sys/vfs.h>
#endif	/* __bsdi__ == 1 || __NetBSD__ == 1 */
#endif	/* BSD4 == 2 || SYS_VFS == 1 */

#if	BSD4 >= 3 && SYS_VFS != 1 && SYS_STATFS != 1
#include	<sys/mount.h>

int	FreeBSize	= 1024;
#endif	/* BSD4 >= 3 && SYS_VFS != 1 && SYS_STATFS != 1 */

Ullong	FreeFSpace;



bool
FSFree(file, space)
	char *			file;	/* On file-system needed */
	Ulong			space;	/* Bytes needed */
{
	Ullong			l1, l2;

#	if	SYSTEM == 5 && SYS_STATFS != 1 && SYS_STATVFS != 1 && SYS_VFS != 1
	struct ustat		buf;
	struct stat		statb;

	while ( stat(file, &statb) == SYSERROR )
		Syserror(CouldNot, StatStr, file);

	if ( ustat(statb.st_dev, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "ustat", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

#	define	fragm_size	statb.st_dev
#	define	fragm_size_name	"dev"
#	define	block_size	FreeBSize
#	define	block_size_name	"FreeBSize"
#	define	block_free	buf.f_tfree
#	define	block_free_name	"f_tfree"

#	endif	/* SYSTEM == 5 && SYS_STATFS != 1 && SYS_STATVFS != 1 && SYS_VFS != 1 */

#	if	BSD4 == 2 || SYS_VFS == 1
	struct statfs	buf;

	if ( statfs(file, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statfs", file);
		FreeFSpace = MAX_ULONG;
		return true;
	}

	if ( buf.f_bavail < 0 )	/* f_bavail is defined as a (long) */
		buf.f_bavail = 0;

#	define	fragm_size	buf.f_bsize
#	define	fragm_size_name	"f_bsize"
#	define	block_free	buf.f_bavail
#	define	block_free_name	"f_bavail"

#	ifdef	__bsdi__
	/* Whatever else, `f_bsize' is not the size of `f_bavail'! */
#	define	block_size	FreeBSize
#	define	block_size_name	"FreeBSize"
#	else	/* __bsdi__ */
#	define	block_size	buf.f_bsize
#	define	block_size_name	"f_bsize"
#	endif	/* __bsdi__ */

#	endif	/* BSD4 == 2 || SYS_VFS == 1 */

#	if	SYS_STATFS == 1
	struct statfs	buf;

	if ( statfs(file, &buf, sizeof(struct statfs), 0) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( errno == EFAULT )
			Syserror(CouldNot, "statfs", file);
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statfs", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

	/* Whatever else, `f_bsize' is not the size of `f_bavail'! */

#	define	block_size	FreeBSize
#	define	block_size_name	"FreeBSize"

#	if	AIX == 1
#	define	fragm_size	buf.f_bsize
#	define	fragm_size_name	"f_bsize"
#	endif	/* AIX == 1 */

#	if	__osf__
#	define	fragm_size	buf.f_fsize
#	define	fragm_size_name	"f_fsize"
#	endif	/* __osf__ */

#	ifndef	fragm_size
#	define	fragm_size	buf.f_frsize
#	define	fragm_size_name	"f_frsize"
#	endif	/* fragm_size */

#	if	sgi == 1 || scounix == 1
#	define	block_free	buf.f_bfree
#	define	block_free_name	"f_bfree"
#	endif	/* sgi == 1 || scounix == 1 */

#	if	QNX == 1
#	define	block_free	buf.f_bfree
#	define	block_free_name	"f_bfree"
#	endif	/* QNX == 1 */

#	ifndef	block_free
#	define	block_free	buf.f_bavail
#	define	block_free_name	"f_bavail"
#	endif	/* !block_free */

#	endif	/* SYS_STATFS == 1 */

#	if	SYS_STATVFS == 1
	struct statvfs	buf;

	if ( statvfs(file, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( errno == EFAULT )
			Syserror(CouldNot, "statvfs", file);
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statvfs", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

#	define	fragm_size	buf.f_bsize
#	define	fragm_size_name	"f_bsize"
#	define	block_size	buf.f_frsize
#	define	block_size_name	"f_frsize"
#	define	block_free	buf.f_bavail
#	define	block_free_name	"f_bavail"

#	endif	/* SYS_STATVFS == 1 */

#	if	BSD4 >= 3 && SYS_VFS != 1 && SYS_STATFS != 1
	struct fs_data	buf;

	if ( statfs(file, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statfs", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

	/* Whatever else, `fd_bsize' is not the size of `fd_bfreen'! */

#	define	fragm_size	buf.fd_bsize
#	define	fragm_size_name	"fd_bsize"
#	define	block_size	FreeBSize
#	define	block_size_name	"FreeBSize"
#	define	block_free	buf.fd_bfreen
#	define	block_free_name	"fd_bfreen"

#	endif	/* BSD4 >= 3 && SYS_VFS != 1 && SYS_STATFS != 1 */

	l1 = (Ullong)block_free * (Ullong)block_size;
	l2 = (Ullong)MINSPOOLFSFREE * (Ullong)1024;
	if ( l2 < l1 )
		FreeFSpace = l1 - l2;
	else
		FreeFSpace = 0;

#	if	LONG_LONG
	Trace10(
		2,
		"FSFree(%s, %lu): %s=%lu, %s=%lu, %s=%lu, free %llu",
		file, (PUlong)space,
		fragm_size_name, (PUlong)fragm_size,
		block_size_name, (PUlong)block_size,
		block_free_name, (PUlong)block_free,
		FreeFSpace
	);
#	else	/* LONG_LONG */
	Trace10(
		2,
		"FSFree(%s, %lu): %s=%lu, %s=%lu, %s=%lu, free %lu",
		file, (PUlong)space,
		fragm_size_name, (PUlong)fragm_size,
		block_size_name, (PUlong)block_size,
		block_free_name, (PUlong)block_free,
		FreeFSpace
	);
#	endif	/* LONG_LONG */

	if ( FreeFSpace <= space )
	{
		Trace4(1, "FSFree(%s, %lu) LOW FREE SPACE %lu", file, (PUlong)space, (PUlong)FreeFSpace);
		return false;
	}

	return true;
}
