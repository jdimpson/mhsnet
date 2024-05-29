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

/*#define _XOPEN_SOURCE*/

/*
**	Include local peculiarities here.
*/

#ifdef	HP_INCLUDE_BUG
#include	<sys/stdsyms.h>
#endif	/* HP_INCLUDE_BUG */
#ifdef	QNX
#include	<unix.h>
#endif	/* QNX */

#if	defined(__STDC__) || defined(__STDC_HOSTED__)
#undef	ANSI_C
#define	ANSI_C	1
#endif	/* __STDC__ */

#ifdef	NOT_ANSI_C
#undef	ANSI_C
#endif	/* NOT_ANSI_C */

#ifdef	ANSI_C
#undef	STDIO
#define	STDIO	1
#undef	STDARGS
#define	STDARGS	1
#endif	/* ANSI_C */

#include	"site.h"

/*
**	Global definitions for magic numbers.
*/

#define	CHST_TTD	(30*60)			/* Time-to-die for state up/down messages */
#define	CRC_SIZE	sizeof(Crc_t)		/* Number of bytes in 16 bit CRC */
#define	CRC32_SIZE	sizeof(Crc32_t)		/* Number of bytes in 32 bit CRC */
#define	DATESTRLEN	13			/* Size of buffer for DateString() */
#define	DATETIMESTRLEN	18			/* Size of buffer for DateTimeStr() */
#define	DAY		(24*HOUR)
#define	HOUR		(60*MINUTE)
#define	LOCK_SLEEP	20			/* Seconds to sleep between lock attempts */
#define	MINUTE		60L
#define	MONTH		(4*WEEK)
#define	NUMERICARGLEN	(ULONG_SIZE+2)		/* Size of buffer for NumericArg() */
#define	TIMESTRLEN	8			/* Size of buffer for TimeString() */
#define	TIME_SIZE	10			/* Maximum number of digits in a decimal time */
#define	ULONG_SIZE	11			/* Maximum bytes in an ASCII `%lu' field */
#define	UNIQUE_NAME_LENGTH 14			/* Number of chars for a unique file name */
#define	WEEK		(7*DAY)
#define	YEAR		(365*DAY)

/*
**	Global definitions for C programming.
*/

#define	BYTE(A)		(((Uchar *)&(A))[0])
#define	english(S)	S			/* Replace with language specific string */
#define	Extern		extern			/* #define to <null> to declare a global object */
#define	NULLFUNCP	(Funcp)0
#define	NULLSTR		(char*)0
#define	NULLVFUNCP	(vFuncp)0
#define	STREQUAL	0
#define	SYSERROR	(-1)
#define	Talloc(T)	(T*)Malloc(sizeof(T))
#define	Talloc0(T)	(T*)Malloc0(sizeof(T))

#define	LOCRC(A)	((A)&0xff)
#define	HICRC(A)	(((A)>>8)&0xff)

#define	CRC32BYT0(A)	((A)&0xff)
#define	CRC32BYT1(A)	(((A)>>8)&0xff)
#define	CRC32BYT2(A)	(((A)>>16)&0xff)
#define	CRC32BYT3(A)	(((A)>>24)&0xff)

/*
**	Type definition for variable argument parameters.
*/

#define	MAXVARARGS	30

typedef struct
{
	int	va_count;
	char *	va_args[MAXVARARGS+1];
}
			VarArgs;

#define	ARG(A,N)	(A)->va_args[N]
#define	FIRSTARG(A)	(A)->va_count=1,(A)->va_args[0]
#define	LASTARG(A)	(A)->va_args[(A)->va_count-1]
#define	NARGS(A)	(A)->va_count
#define	NEXTARG(A)	(A)->va_args[(A)->va_count++]

#ifdef	RECOVER

/*
**	Type definition for error recovery.
*/

typedef enum { ert_finish, ert_here, ert_return, ert_exit } ERC_t;

#include	<setjmp.h>

extern jmp_buf		ErrBuf;
extern ERC_t		ErrFlag;

#define	RecoverV(T)	(ErrFlag=T,((T==ert_here)?setjmp(ErrBuf):0))
#define	Recover(T)	(void)RecoverV(T)

#endif	/* RECOVER */

/*
**	System include file control.
*/

#ifdef	NEED_HZ
#ifndef	QNX
#include	<sys/param.h>
#endif	/* !QNX */
#ifndef	HZ
#define	HZ	100		/* A stab in the dark */
#endif	/* HZ */
#ifdef	major
#define	_TYPES_
#endif	/* major */
#endif	/* NEED_HZ */

#ifdef	SIGNALS
#ifndef	SIG_IGN
#include	<signal.h>
#endif
#if	KILL_0 == 1
#define	SIG0		0
#else
#define	SIG0		SIGINT
#endif
#ifdef	SIGUSR1
#define	SIGWAKEUP	SIGUSR1
#else
#define	SIGWAKEUP	SIGEMT
#endif
#ifdef	SIGUSR2
#define	SIGERROR	SIGUSR2
#else
#define	SIGERROR	SIGIOT
#endif
#if	BSD4 >= 3
#define	SIGBLOCK	1
#endif	/* BSD4 >= 3 */
#if	SYSVREL >= 4 && QNX == 0
#define	signal		sigset
#else	/* SYSVREL >= 4 && QNX == 0 */
#ifdef	SIGHOLD
#define	signal		sigset
#endif	/* SIGHOLD */
#endif	/* SYSVREL >= 4 && QNX == 0 */
#endif	/* SIGNALS */

#ifdef	EBUG
#define	DEBUG		EBUG
#else
#define	DEBUG		0
#endif

#ifdef	LOCKING
#undef	FILE_CONTROL
#define	FILE_CONTROL
#endif

#ifdef	FILE_CONTROL
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#if	FCNTL == 1
#include	<fcntl.h>	/* O_XXX definitions may live here */
#ifndef	_FCNTL_
#define	_FCNTL_
#endif	/* _FCNTL_ */
#if	BSD4 >= 2
#include	<sys/file.h>	/* O_XXX definitions live here */
#ifndef	_FILE_
#define	_FILE_
#endif	/* _FILE_ */
#endif	/* BSD4 >= 2 */
#else	/* FCNTL == 1 */
#if	BSD4 > 0
#ifndef	O_RDWR
#include	<sys/file.h>	/* O_XXX definitions live here */
#ifndef	_FILE_
#define	_FILE_
#endif	/* _FILE_ */
#endif	/* O_RDWR */
#if	BSD4 < 2
#undef	O_EXCL			/* Does the wrong thing */
#endif	/* BSD4 < 2 */
#else	/* BSD4 > 0 */
#if	V8 == 1
#include	<sys/ioctl.h>
#endif	/* V8 */
#define	O_READ		0
#define	O_WRITE		1
#define	O_RDWR		2
#define	O_APPEND	0
#endif	/* BSD4 > 0 */
#ifndef	O_EXCL
#define	O_EXCL		0
#endif	/* O_EXCL */
#endif	/* FCNTL == 1 */
#ifndef	O_READ
#define	O_READ	O_RDONLY
#endif	/* O_READ */
#ifndef	O_WRITE
#define	O_WRITE	O_WRONLY
#endif	/* O_WRITE */
#endif	/* FILE_CONTROL */

#ifdef	SELECT
#if	V8 == 1 || BSD4 >= 2
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#ifndef	FD_SET			/* missing definitions for "select" in 4.2 */
/*
**	If your <sys/types.h> typedef's fd_set, but doesn't define FD_SET,
**	then remove the following typedef...
**
**	These assume fd < 32, that is true on 4.2bsd, the only kernel
**	that has "select" but didn't bother to provide these defn's...
*/
#if	0
typedef struct { long fds_bits[1]; } fd_set;	/* If problem -- read above! */
#endif	/* 0 */
#define	FD_SET(fd, set)		((set)->fds_bits[0] |= (1 << (fd)))
#define	FD_CLR(fd, set)		((set)->fds_bits[0] &= ~(1 << (fd)))
#define	FD_ISSET(fd, set)	((set)->fds_bits[0] & (1 << (fd)))
#define	FD_ZERO(set)		(strclr((char *)(set), sizeof(*(set))))
#endif	/* FD_SET */
#endif	/* V8 == 1 || BSD4 >= 2 */
#endif	/* SELECT */

#ifdef	STAT_CALL
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#include	<sys/stat.h>	/* Definition of stat structure */
#endif	/* STAT_CALL */

#ifdef	WAIT_CALL
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#include	<sys/wait.h>	/* Definition of wait syscall */
#endif	/* WAIT_CALL */

#ifdef	PASSWD_USED
#if	AUSAS == 1
#undef	SYSERROR
#undef	V8
#include	<local-system>
#ifndef	SYSERROR
#define	SYSERROR	(-1)
#endif	/* SYSERROR */
#include	<passwd.h>
#else	/* AUSAS == 1 */
#include	<pwd.h>
#endif	/* AUSAS == 1 */
#endif	/* PASSWD_USED */

#ifdef	STDIO
#include	<stdio.h>
#endif	/* STDIO */

#ifdef	SMDATE
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#endif	/* SMDATE */

#ifdef	TIME
#include	<time.h>
#endif	/* TIME */

#ifdef	TIMES
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#include	<sys/times.h>
#endif	/* TIMES */

#ifdef	TERMIOCTL
#if	SYSTEM >= 3
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#include	<termio.h>
#else	/* SYSTEM >= 3 */
#include	<sgtty.h>
#ifdef	TIOCGETP
#ifndef gtty
#define	gtty(A,B)	ioctl(A, TIOCGETP, B)
#define	stty(A,B)	ioctl(A, TIOCSETP, B)
#endif	/* gtty */
#endif	/* TIOCGETP */
#endif	/* SYSTEM >= 3 */
#endif	/* TERMIOCTL */

#ifdef	LOCKING
#if	AUTO_LOCKING == 1
#define	Lock(A,B,C)	(C==for_reading?readlock(B):writelock(B))
#define	UnLock(B)	unlock()
#else	/* AUTO_LOCKING == 1 */
#if	FLOCK == 1
#if	!defined(FEXLOCK) && !defined(LOCK_EX) && !defined(F_SETLKW)
#ifndef	_TYPES_
#include	<sys/types.h>
#ifndef	_TYPES_
#define	_TYPES_
#endif	/* _TYPES_ */
#endif	/* _TYPES_ */
#ifndef	_FILE_
#include	<sys/file.h>
#endif	/* _FILE_ */
#ifndef	O_READ
#define	O_READ	O_RDONLY
#endif	/* O_READ */
#ifndef	O_WRITE
#define	O_WRITE	O_WRONLY
#endif	/* O_WRITE */
#endif	/* !defined(FEXLOCK) && !defined(LOCK_EX) && !defined(F_SETLKW) */
#if BSD4 == 1 && BSD4V == 'c'
#define	Lock(A,B,C)	flock(B,C==for_writing?FEXLOCK:FSHLOCK)
#define	UnLock(B)
#else	/* BSD4 == 1 && BSD4V == 'c' */
#ifndef	F_SETLKW
#define	Lock(A,B,C)	flock(B,C==for_writing?LOCK_EX:LOCK_SH)
#define	UnLock(B)	flock(B, LOCK_UN)
#else	/* F_SETLKW */
#define	Lock(A,B,C)	_Lock(A,B,C)
#define	UnLock(B)	_UnLock(B)
#endif	/* F_SETLKW */
#endif	/*BSD4 == 1 && BSD4V == 'c' */
#else	/* FLOCK == 1 */
#if	SYS_LOCKING == 1
#define	Lock(A,B,C)	locking(B,1,0)
#define	UnLock(B)	locking(B,0,0)
#else	/* SYS_LOCKING == 1 */
#if	LOCKF	== 1
#ifndef	F_SETLKW
#define	Lock(A,B,C)	lockf_all(B,1)
#define	UnLock(B)	lockf_all(B,0)
#else	/* F_SETLKW */
#define	Lock(A,B,C)	_Lock(A,B,C)
#define	UnLock(B)	_UnLock(B)
#endif	/* F_SETLKW */
#else	/* LOCKF == 1 */
#if	V8 == 1
#ifndef	_IOCTL_
#include	<sys/ioctl.h>
#endif	/* _IOCTL_ */
#define	Lock(A,B,C)	_Lock(A,B,C)
#define	UnLock(B)	(void)ioctl(B,FIOAUNLOCK,0)
#else	/* V8 == 1 */
#define	Lock(A,B,C)	_Lock(A,B,C)
#define	UnLock(B)	_UnLock()
#endif	/* V8 == 1 */
#endif	/* LOCKF == 1 */
#endif	/* SYS_LOCKING == 1 */
#endif	/* FLOCK == 1 */
#endif	/* AUTO_LOCKING == 1 */
#endif	/* LOCKING */

#ifdef	ERRNO
#ifndef	ERRNO_H
#define	ERRNO_H
#endif	/* ERRNO_H */
#endif	/* ERRNO */

#ifdef	SYS_ERRNO_H
/*
** This _ought_ to be automatically included by <errno.h>,
** but, in some systems, notably HP-UX, it isn't.
*/
#include	<sys/errno.h>
#endif	/* SYS_ERRNO_H */

#ifdef	ERRNO_H
#include	<errno.h>
#endif	/* ERRNO_H */

#ifdef	STDARGS
#ifdef	ANSI_C
#include	<stdarg.h>
#else	/* ANSI_C */
#include	<varargs.h>
#endif	/* ANSI_C */
#endif	/* STDARGS */

#ifdef	STDLIB_H
#include	<stdlib.h>
#endif	/* STDLIB_H */

#ifdef	STRING_H
#include	<string.h>
#endif	/* STRING_H */

#ifdef	UNISTD_H
#include	<unistd.h>
#endif	/* UNISTD_H */

#ifdef	LINUX
#include	<search.h>
#endif	/* LINUX */

/*
**	System dependancies.
*/

#if	(BSD4 >= 2 || SYSTEM >= 5 && SYSVREL >= 4 || linux == 1)
#ifndef	FSYNC_2
#define	FSYNC_2	1	/* `fsync(2)' system call present */
#endif	/* FSYNC_2 */
#ifndef	MKDIR_2
#define	MKDIR_2	1	/* `mkdir(2)' system call present */
#endif	/* MKDIR_2 */
#ifndef	RENAME_2	
#define	RENAME_2 1	/* `rename(2)' system call present */
#endif	/* RENAME_2 */
#endif	/* (BSD4 >= 2 || SYSTEM >= 5 && SYSVREL >= 4 || linux == 1) */

/*
**	Include type, function and string definitions here.
*/

#include	"typedefs.h"
#include	"functions.h"
#include	"_strings.h"

#if	DEBUG >= 1
#define	free	Free
#endif	/* DEBUG >= 1 */
