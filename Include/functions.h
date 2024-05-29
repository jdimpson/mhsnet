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


#ifndef	TIME
#include	<time.h>
#endif	/* TIME */

#ifdef	ANSI_C
#define _FA_(A)  A
#else	/* ANSI_C */
#define _FA_(A)  ()
#endif	/* ANSI_C */

/*
**	Network library (Lib.a) definitions.
*/

#define	ErrnoStr(A)	strerror(A)

extern Crc_t	acrc _FA_((Crc_t, char *, int));
extern Crc32_t	acrc32 _FA_((Crc32_t, char *, int));
extern bool	AddressMatch _FA_((char *, char *));
extern bool	CanAddrMatch _FA_((char *, char *));
extern bool	CanReadPipe _FA_((int, char *));
extern bool	CheckDirs _FA_((char *));
extern char *	CleanError _FA_((char *));
extern bool	CopyFdToFd _FA_((int, char *, int, char *, Ulong));
extern bool	CopyFileToFd _FA_((char *, int, char *));
extern char	CouldNot[];
extern void	CpuStats _FA_((FILE *, Time_t));
extern bool	crc _FA_((char *, int));
extern void	crc32 _FA_((char *, int));
extern int	create _FA_((char *));
extern int	createn _FA_((char *));
extern int	creater _FA_((char *));
extern char	CreateStr[];
extern bool	DaemonActive _FA_((char *, bool));
extern char *	DateString _FA_((Time_t, char *));
extern char *	DateTimeStr _FA_((Time_t, char *));
extern Ulong	DecodeNum _FA_((char *, int));
extern int	Detach _FA_((bool, int, bool, bool));
extern char	DevNull[];
extern char *	DomainToPath _FA_((char *));
extern char	Dup[];
extern void	EchoArgs _FA_((int, char **));
extern char	EmptyString[];
extern int	EncodeNum _FA_((char *, Ulong, int));
extern bool	ErrIsatty;
extern bool	ErrorTty _FA_((int *));
extern int	ErrSize;
extern char *	ErrString;
extern void	ExpandArgs _FA_((VarArgs *, char **));
extern void	ExpandFile _FA_((FILE *, char *, int));
extern char *	ExpandString _FA_((const char *, int));
extern int	ExStatus;
extern void	finish _FA_((int));
extern char	ForkStr[];
extern void	Free _FA_((char *));
extern void	FreeStr _FA_((char **));
extern Ullong	FreeFSpace;
extern bool	FSFree _FA_((char *, Ulong));
extern char *	GetErrFile _FA_((VarArgs *, int, int));
extern void	InitParams _FA_((void));
extern bool	IsattyDone;
extern char *	KeepErrFile;
extern void	listsort _FA_((char **, int (*)(char *, char *)));
extern char *	LockNode;
extern int	LockPid;
extern char	LockStr[];
extern Time_t	LockTime;
extern void	MakeErrFile _FA_((int *));
extern bool	MakeLock _FA_((char *));
extern void	make_link _FA_((char *, char *));
extern char *	MailNCC _FA_((vFuncp, char *));
extern char *	Malloc _FA_((unsigned));
extern char *	Malloc0 _FA_((unsigned));
extern void	MesgV _FA_((char *, char *, va_list));
extern Time_t	millisecs _FA_((void));
extern bool	MkDirs _FA_((char *, int, int));
extern void	move _FA_((char *, char *));
extern char *	Name;
extern char *	newnstr _FA_((const char *, int));
extern char *	newstr _FA_((const char *));
extern char *	newvprintf _FA_((char *, va_list));
extern char *	NodeName _FA_((void));
extern char	NullStr[];
extern char *	NumericArg _FA_((char *, int, Ulong));
extern char	OpenStr[];
extern long	otol _FA_((char *));
extern int	Pid;
extern char	PipeStr[];
extern void	QuoteChars _FA_((char *, char *));
extern unsigned	RdFileSize;
extern Time_t	RdFileTime;
extern char *	ReadErrFile _FA_((char *, bool));
extern char *	ReadFd _FA_((int));
extern char *	ReadFile _FA_((char *));
extern char	ReadStr[];
extern int	ReRoute _FA_((char *, char *));
extern void	ResetErrTty _FA_((void));
extern bool	ReSetLock _FA_((char *, int, int));
extern char *	rfc822time _FA_((time_t *));
extern bool	RmDir _FA_((char *));
extern char	SeekStr[];
extern bool	SetLock _FA_((char *, int, bool));
extern void	SetNameProg _FA_((int, char **, char ** ));
extern char	SetQuality _FA_((int, Ulong));
extern int	SetRaw _FA_((int, int, int, int, bool));
extern void	SetUlimit _FA_((void));
extern void	SetUser _FA_((int, int));
extern char	Slash[];
extern unsigned	sleep _FA_((unsigned));
extern void	SMdate _FA_((char *, Time_t));
extern char	SObuf[];
extern int	SplitArg _FA_((VarArgs *, char *));
extern int	SpoolDirLen;
extern char	StatStr[];
extern void	StderrLog _FA_((char *));
extern int	strccmp _FA_((const char *, const char *));
extern char *	strclr _FA_((char *, int));
extern char *	strcpyend _FA_((char *, char *));
extern char *	StringCRC32 _FA_((char *));
extern char	StringFmt[];
extern char *	StringMatch _FA_((char *, char *));
extern char *	StripCopyEnd _FA_((char *, char *));
extern char *	StripDomain _FA_((char *, char *, char *, bool));
extern char *	StripDEnd _FA_((char *, char *, char *, char *, bool));
extern char **	StripEnv _FA_((void));
extern char *	StripErrString _FA_((char *));
extern char *	StripTypes _FA_((char *));
extern int	strnccmp _FA_((char *, char *, int));
extern char *	strncpyend _FA_((char *, char *, int));
extern char *	strrpbrk _FA_((char *, char *));
extern bool	SysRetry _FA_((int));
extern bool	tcrc _FA_((char *, int));
extern bool	tcrc32 _FA_((char *, int));
extern Time_t	ticks _FA_((void));
extern Time_t	Time;
extern char *	TimeString _FA_((Time_t, char *));
extern char *	ToLower _FA_((char *, int));
extern char *	ToUpper _FA_((char *, int));
extern long	Ulimit;
extern char *	UniqueName _FA_((char *, int, bool, Ulong, Time_t));
extern char	Unknown[];
extern char	UnlinkStr[];
extern void	UnQuoteChars _FA_((char *, char *));
extern char	Version[];
extern char	WriteStr[];
extern long	xtol _FA_((char *));
extern int	_Lock _FA_((char *, int, Lock_t));
extern void	_UnLock _FA_((int));

/*
**	STDARGS routines.
*/

extern char *	concat _FA_((char *, ...));
extern void	Error _FA_((char *, ...));
extern bool	InList _FA_((bool(*)(char *, char *), char *, char *, ...));
extern void	MesgN _FA_((char *, ...));
extern void	NameProg _FA_((char *, ...));
extern char *	newprintf _FA_((char *, ...));
extern char *	regcmp _FA_((char *, ...));
extern char *	regex _FA_((char *, char *, ...));
extern void	Syserror _FA_((char *, ...));
extern bool	SysWarn _FA_((char *, ...));
extern void	Warn _FA_((char *, ...));

/*
**	C-library definitions.
*/

#ifndef	Rand
#define	Rand	rand
#define	SRand	srand
#endif

#if	!defined(QNX) && !defined(ERRNO_H)
#define	_ERRNO_H
extern int	errno;
#endif	/* !defined(QNX) && !defined(ERRNO_H) */

#if	!defined(QNX) && !defined(STDLIB_H)
#define	_STDLIB_H
extern double	atof _FA_((const char *));
extern long	atol _FA_((const char *));
extern char *	bsearch _FA_((const void *, const void *, unsigned, unsigned, int (*)(char *, char *)));
extern void	free _FA_((void *));
extern char *	getenv _FA_((const char *));
extern char *	lfind _FA_((char *, char *, unsigned *, unsigned, int (*)(char *, char *)));
#if	defined(__STDC__) || defined(__STDC_HOSTED__)
extern void *	malloc _FA_((unsigned));
extern void *	calloc _FA_((unsigned, unsigned));
extern void *	realloc _FA_((void *, size_t));
#else	/* defined(__STDC__) */
extern char *	malloc _FA_((unsigned));
extern char *	calloc _FA_((unsigned, unsigned));
extern char *	realloc _FA_((char *, unsigned));
#endif	/* defined(__STDC__) */
#endif	/* !defined(QNX) && !defined(STDLIB_H) */

#if	!defined(QNX) && !defined(STRING_H)
#define	_STRING_H
extern void	bcopy _FA_((const char *, char *, int));
extern char *	strcat _FA_((char *, const char *));
extern char *	strchr _FA_((const char *, unsigned));
extern char *	strcpy _FA_((char *, const char *));
extern char *	strerror _FA_((int));
extern char *	strncpy _FA_((char *, const char *, unsigned));
extern char *	strpbrk _FA_((const char *, const char *));
extern char *	strrchr _FA_((const char *, unsigned));
extern size_t	strspn _FA_((const char *, const char *));
extern char *	strtok _FA_((char *, const char *));
#endif	/* !defined(QNX) && !defined(STRING_H) */

#if	!defined(QNX) && !defined(UNISTD_H) && !defined(_H_UNISTD)
#define	_UNISTD_H
#if	BSDI < 2
extern long	lseek _FA_((int, long, int));
#endif	/* BSDI < 2 */
#ifndef	_TIME_T
extern char *	ctime _FA_((const long *));
extern long	time _FA_((long *));
#endif	/* _TIME_T */
extern char *	sbrk _FA_((int));
extern int	write _FA_((int, char *, int));
#endif	/* !defined(QNX) && !defined(UNISTD_H) */
