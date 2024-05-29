/*
** UUCP compatibilty code.
**
**	Make a uucp compatible device lock file, to prevent
**	uucp using a line simultaneously with netcall.
**
**	(As tip, cu, and relevant ugate's all do the same
**	things, compatibility is ensured with all.)
**
**	SCCSID @(#)uucplock.c	1.26 09/07/30
*/

#define	FILE_CONTROL
#define	SIGNALS
#define	STDIO
#define	STAT_CALL

#include "global.h"

#include <ctype.h>
#include <utime.h>

#if	SYSMACROS_H == 1
#include <sys/sysmacros.h>
#endif	/* SYSMACROS_H == 1 */

#if	MKDEV_H == 1
#include <sys/mkdev.h>
#endif	/* MKDEV_H == 1 */

#ifndef	UUCPLOCKLIB

#ifndef	UUCPLCKPRE
#if	UUCPMLOCKDEV == 1
#define	UUCPLCKPRE	"LK."
#else	/* UUCPMLOCKDEV == 1 */
#define	UUCPLCKPRE	"LCK.."
#endif	/* UUCPMLOCKDEV == 1 */
#endif	/* !UUCPLCKPRE */

#ifndef	UUCPLCKDIR
#define	UUCPLCKDIR	"/usr/spool/uucp/"	/* /usr/spool/uucp/LCK for BSD4.3+ */
#endif	/* !UUCPLCKDIR */

extern int	UUCPlowerdev;
extern int	UUCPmlockdev;
extern int	UUCPstrpid;
extern char *	UUCPlockfile;
static char	mlckfmt[]	= "%s%s%3.3lu.%3.3lu.%3.3lu"; /* UUCPLOCKDIR, UUCPLCKPRE, maj(dev), maj(rdev), min(rdev) */
static char	lckfmt[]	= "%s%s%s";	/* UUCPLOCKDIR, UUCPLCKPRE, tty */

#define NAMESIZE 50
#define FAIL -1
#define SAME 0
#define SLCKTIME 28800   /* system/device timeout (LCK.. files) in seconds */

#define	PIDSTRSIZE	11

#if	DEBUG >= 2
#if	ANSI_C
#define ASSERT(e, f, v) if (!(e)) {\
	fprintf(stderr, "AERROR - (%s) ", #e);\
	fprintf(stderr, f, v);\
	rmlock(NULL);\
	exit(1);\
}
#else	/* ANSI_C */
#define ASSERT(e, f, v) if (!(e)) {\
	fprintf(stderr, "AERROR - (%s) ", "e");\
	fprintf(stderr, f, v);\
	rmlock(NULL);\
	exit(1);\
}
#endif	/* ANSI_C */
#else	/* DEBUG >= 2 */
#define ASSERT(e, f, v)
#endif	/* DEBUG >= 2 */

/* #include "uucp.h" */
/* #include <sys/types.h> */
/* #include <sys/stat.h> */

void		rmlock _FA_((char *));
void		stlock _FA_((char *));
static int	onelock _FA_((int, char *, char *));



/*******
 *      ulockf(file, atime)
 *      char *file;
 *      time_t atime;
 *
 *      ulockf  -  this routine will create a lock file (file).
 *      If one already exists, the create time is checked for
 *      older than the age time (atime).
 *      If it is older, an attempt will be made to unlink it
 *      and create a new one.
 *
 *      return codes:  0  |  FAIL
 */

int
ulockf(file, atime)
char *file;
time_t atime;
{
	struct stat stbuf;
	time_t ptime;
	static char tempfile[NAMESIZE];

	if (tempfile[0] == '\0')
		sprintf(tempfile, "%sLTMP.%d", UUCPLCKDIR, Pid);
	if (onelock(Pid, tempfile, file) == -1) {
		/* lock file exists */
		/* get status to check age of the lock file */
		if (stat(file, &stbuf) != -1) {
			time(&ptime);
			if ((ptime - stbuf.st_ctime) < atime) {
				/* file not old enough to delete */
#				if	KILL_0 == 1
				/* try checking pid */
				char	pidbuf[PIDSTRSIZE+1];	/* "nnnnnnnnnn\n\0" */
				int	pid;
				int	fd;

				if ((fd = open(file, O_RDONLY)) == -1)
					return(FAIL);
				if ( UUCPstrpid == 1 ) {
					if (stbuf.st_size > sizeof pidbuf) {
						(void)close(fd);
						return(FAIL);
					}
					if ((pid = read(fd, pidbuf, PIDSTRSIZE)) <= 0) {
						(void)close(fd);
						return(FAIL);
					}
					(void)close(fd);
					pidbuf[pid] = '\0';
					if ((pid = atoi(pidbuf)) == 0)
						return(FAIL);
				} else {
					if (read(fd, (char *)&pid, sizeof pid) != sizeof pid) {
						(void)close(fd);
						return(FAIL);
					}
					(void)close(fd);
					if (pid == 0)
						return(FAIL);
				}
				if (kill(pid, SIG0) != -1 || errno != ESRCH)
#				endif	/* KILL_0 == 1 */
					return(FAIL);
			}
		}
		(void)unlink(file);
		if (onelock(Pid, tempfile, file) == -1)
			return(FAIL);
	}
	stlock(file);
	return(0);
}


#define MAXLOCKS 10     /* maximum number of lock files */
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

/***
 *      stlock(name)    put name in list of lock files
 *      char *name;
 *
 *      return codes:  none
 */

void
stlock(name)
char *name;
{
	char *p;
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			break;
	}
	ASSERT(i < MAXLOCKS, "TOO MANY LOCKS %d", i);
	if (i >= Nlocks)
		i = Nlocks++;
	p = calloc(strlen(name) + 1, sizeof (char));
	ASSERT(p != NULL, "CAN NOT ALLOCATE FOR %s", name);
	strcpy(p, name);
	Lockfile[i] = p;
}


/***
 *      rmlock(name)    remove all lock files in list
 *      char *name;     or name
 *
 *      return codes: none
 */

void
rmlock(name)
char *name;
{
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		if (name == NULL
		|| strcmp(name, Lockfile[i]) == SAME) {
			unlink(Lockfile[i]);
			free(Lockfile[i]);
			Lockfile[i] = NULL;
		}
	}
}


/*  this stuff from pjw  */
/*  /usr/pjw/bin/recover - check pids to remove unnecessary locks */
/*      onelock(pid,tempfile,name) makes lock a name
	on behalf of pid.  Tempfile must be in the same
	file system as name. */
/*      lock(pid,tempfile,names) either locks all the
	names or none of them */
static int
onelock(pid,tempfile,name) int pid; char *tempfile,*name;
{
	int		fd;
	char		pidbuf[PIDSTRSIZE+1];	/* "nnnnnnnnnn\n\0" */

	fd=creat(tempfile,0444);
	if(fd<0) return(-1);
#if BSD4 > 1
	fchown(fd, getuid(), getgid());
	fchmod(fd, 0444);
#else
	chown(tempfile, getuid(), getgid());
	chmod(tempfile, 0444);
#endif
	if ( UUCPstrpid == 1 ) {
		(void)sprintf(pidbuf, "%*d\n", PIDSTRSIZE-1, pid);
		(void)write(fd, pidbuf, PIDSTRSIZE);
	} else
		write(fd,(char *) &pid, sizeof(int));
	close(fd);
	if(link(tempfile,name)<0)
	{
		unlink(tempfile);
		return(-1);
	}
	unlink(tempfile);
	return(0);
}


/***
 *      lockname(file, buf)      create system lock name
 *      char *file;
 *      char *buf;
 *
 *      return codes:  new lockname (in `buf')
 */

static char *
lockname(file, buf)
char *file;
char *buf;
{
	register char *	cp;
	register int	c;
	char		lowerfile[NAMESIZE];
	struct stat	statb;

	if ( UUCPmlockdev )
	{
		(void)strcpy(buf, DevNull);	/* /dev/null */
		(void)strcpy(&buf[5], file);	/* /dev/<file> */

		if ( stat(buf, &statb) == SYSERROR )
			return NULLSTR;

		(void)sprintf
		(
			buf,
			mlckfmt,
			UUCPLCKDIR,
			UUCPLCKPRE,
			(PUlong)major(statb.st_dev),
			(PUlong)major(statb.st_rdev),
			(PUlong)minor(statb.st_rdev)
		);
	}
	else
	if ( UUCPlowerdev )
	{
		cp = strcpyend(lowerfile, file);
		c = *--cp;
		if ( isupper(c) )
			*cp = tolower(c);
		(void)sprintf(buf, lckfmt, UUCPLCKDIR, UUCPLCKPRE, lowerfile);
	}
	else
		(void)sprintf(buf, lckfmt, UUCPLCKDIR, UUCPLCKPRE, file);

	FreeStr(&UUCPlockfile);
	UUCPlockfile = newstr(buf);

	return buf;
}

/***
 *      delock(s)       remove a lock file
 *      char *s;
 *
 *      return codes:  0  |  FAIL
 */

#if	0
delock(s)
char *s;
{
	char *	cp;
	char	ln[NAMESIZE];

	if ( lockname(s, ln) != NULLSTR )
		rmlock(ln);
}
#endif	/* 0 */


/***
 *      mlock(sys)      create system lock
 *      char *sys;
 *
 *      return codes:  0  |  FAIL
 */

int
mlock(sys)
char *sys;
{
	char	lname[NAMESIZE];

	if ( lockname(sys, lname) == NULLSTR )
		return FAIL;

	return ulockf(lname, (time_t) SLCKTIME ) < 0 ? FAIL : 0;
}



/***
 *      ultouch()       update access and modify times for lock files
 *
 *      return code - none
 */

void
ultouch()
{
	/* time_t time(); */
	int i;
	struct utimbuf ut;

	ut.actime = time(&ut.modtime);
	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		utime(Lockfile[i], &ut);
	}
}

#else	/* UUCPLOCKLIB */
void _dummy_uucplocks(){}
#endif	/* UUCPLOCKLIB */
