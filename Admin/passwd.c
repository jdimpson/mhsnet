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


static char	sccsid[]	= "%W% %E%";

/*
**	Add/change network passwords.
*/

#define	STAT_CALL
#define	FILE_CONTROL
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	TERMIOCTL
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


#include	<crypt.h>



/*
**	Parameters set from arguments.
*/

bool	Check;			/* Check password */
char *	Comment;		/* Comment for entry */
bool	Delete;			/* Delete password */
char *	Name	= sccsid;
bool	NullPass;		/* Create null password */
char *	OrigName;		/* Address as given */
char *	PassName;		/* Address in canonical form */
bool	Print;			/* Show password file contents */
int	Traceflag;

/*
**	Arguments.
*/

AFuncv	getAddress _FA_((PassVal, Pointer));	/* Convert address to internal form */
AFuncv	getComment _FA_((PassVal, Pointer));	/* Accumulate comments for password entry */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(c, &Check, 0),
	Arg_bool(d, &Delete, 0),
	Arg_bool(p, &Print, 0),
	Arg_bool(n, &NullPass, 0),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_noflag(0, getAddress, english("address"), OPTARG),
	Arg_noflag(0, getComment, english("comment"), OPTARG|MANY),
	Arg_end
};


/*
**	Sort command for passwd file.
**
**	This is settable via PASSWDSORT:
**	  it must sort the passwd file on the leading chars after the first period,
**	  with input taken from the file named by the first `%s',
**	  and output sent to the file named by the second `%s'.
**
**char *PasswdSortStr	= "sort -t. -k2nr %s -o %s"
*/

/*
**	Miscellaneous definitions.
*/

char	DevTty[] = "/dev/tty";
Passwd	Invoker;		/* Person invoked by */
char *	LockFile;		/* Lock passwd file during re-write */
char *	TmpFile;		/* New passwd file during re-write */
int	PasswdFd;		/* `passwd' file descriptor */
int	fd;			/* Current file descriptor */

char	CryptAlphabet[]	= "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

#if	SYSTEM >= 3
struct termio	params;
#else	/* SYSTEM >= 3 */
struct sgttyb	params;
#endif	/* SYSTEM >= 3 */

#define	PWDSIZE	128			/* Maximum bytes in a password */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"PASSWDSORT",	(char **)&PASSWDSORT,		PSTRING},
	{"TRACEFLAG",	(char **)&Traceflag,		PINT},
};


#define	Printf	(void)printf

void	checkpasswd _FA_((void));
void	echo_on _FA_((void));
void	echo_off _FA_((void));
void	getpasswd _FA_((char *, char *));
void	printpasswd _FA_((void));
void	setpasswd _FA_((void));
void	writefd _FA_((char *, int));

extern SigFunc	sigcatch;


int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	Passwd	pe;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	SetUlimit();

	/** Open DevTty before SetUser() **/

	if ( !NullPass )
	while ( (PasswdFd = open(DevTty, O_RDWR)) == SYSERROR )
	{
		if ( errno == EPERM )
			(void)fprintf(stderr, english("Try \"chmod ug+rw `tty`\""));

		Syserror(CouldNot, OpenStr, DevTty);
	}

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Recover(ert_return);
		Error(english("No permission."));
		exit(EX_NOPERM);
	}

	if ( Print )
	{
		printpasswd();
		exit(EX_OK);
	}

	if ( OrigName == NULLSTR )
	{
		(void)fprintf(stderr, "%s\n", ArgsUsage(Usage));
		exit(EX_USAGE);
	}

	if ( GetUid(&pe, OrigName) && pe.P_crypt[0] != '\0' )
		Warn(english("\"%s\" has a password in /etc/passwd."), OrigName);

	if ( Check )
		checkpasswd();
	else
		setpasswd();

	exit(EX_OK);
	return 0;
}



void
checkpasswd()
{
	char *	encrypted;
	Passwd	pe;
	char	try1[PWDSIZE];

	if ( !GetPassword(&pe, PassName) )
	{
		Printf(english("No entry for \"%s\"\n"), PassName);
		return;
	}

	Printf(english("Checking password for \"%s\"\n"), pe.P_name);

	getpasswd(try1, english("Enter password"));

	Trace4(1, "entered=\"%s\", crypt=\"%.*s\"", try1, sizeof(pe.P_crypt), pe.P_crypt);

	encrypted = crypt(try1, pe.P_crypt);

	Trace2(1, "encrypted=\"%s\"", encrypted);

	if ( pe.P_crypt[0] == '\0' && try1[0] == '\0' )
		encrypted = pe.P_crypt;

	if ( strcmp(pe.P_crypt, encrypted) != STREQUAL )
	{
		Printf(english("Incorrect.\n"));
		finish(EX_TEMPFAIL);
	}

	Printf(english("Correct.\n"));
}



void
echo_off()
{
#	if	SYSTEM >= 3
	while ( ioctl(fd, TCGETA, &params) == SYSERROR )
		Syserror(CouldNot, "TCGETA", DevTty);
	params.c_lflag &= ~ECHO;
	while ( ioctl(fd, TCSETAF, &params) == SYSERROR )
		Syserror(CouldNot, "TCSETAF", DevTty);
	params.c_lflag |= ECHO;
#	else	/* SYSTEM >= 3 */
	while ( gtty(fd, &params) == SYSERROR )
		Syserror(CouldNot, "gtty", DevTty);
	params.sg_flags &= ~ECHO;
	while ( stty(fd, &params) == SYSERROR )
		Syserror(CouldNot, "stty", DevTty);
	params.sg_flags |= ECHO;
#	endif	/* SYSTEM >= 3 */
}



void
echo_on()
{
#	if	SYSTEM >= 3
	while ( ioctl(fd, TCSETAF, &params) == SYSERROR )
		Syserror(CouldNot, "TCSETAF", DevTty);
#	else	/* SYSTEM >= 3 */
	while ( stty(fd, &params) == SYSERROR )
		Syserror(CouldNot, "stty", DevTty);
#	endif	/* SYSTEM >= 3 */
}



void
finish(error)
	int	error;
{
	if ( LockFile != NULLSTR )
		(void)unlink(LockFile);

	if ( TmpFile != NULLSTR )
		(void)unlink(TmpFile);

	exit(error);
}



/*
**	Convert site address.
*/

/*ARGSUSED*/
AFuncv
getAddress(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Addr *	ap;
	char *	cp;
	int	l;

	OrigName = newstr(val.p);
	cp = newstr(val.p);

	ap = StringToAddr(cp, NO_STA_MAP);

	PassName = newstr(TypedAddress(ap));

	FreeAddr(&ap);
	free(cp);

	Trace2(1, "PassName \"%s\"", PassName);
	l = strlen(PassName);

	switch ( PassName[0] )
	{
	case ATYP_DESTINATION:
		if ( l < 4 || PassName[2] != TYPE_SEP )
			break;
		return ACCEPTARG;
	case ATYP_BROADCAST:
		if ( PassName[1] != ATYP_DESTINATION )
			break;
		if ( PassName[2] != '\0' && (l < 4 || PassName[3] != TYPE_SEP) )
			break;
		return ACCEPTARG;
	}

	return (AFuncv)english("unrecognised address");
}



/*
**	Accumulate comments.
*/

/*ARGSUSED*/
AFuncv
getComment(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( Comment == NULLSTR )
		Comment = val.p;
	else
		Comment = concat(Comment, " ", val.p, NULLSTR);

	return ACCEPTARG;
}



void
getpasswd(buf, query)
	char *	buf;
	char *	query;
{
	char *	cp;
	char *	ep;
	char	c;

	if ( NullPass )
	{
		*buf = '\0';
		return;
	}

	(void)signal(SIGINT, sigcatch);

	fd = PasswdFd;

	echo_off();
	writefd(query, strlen(query));
	writefd(": ", 2);

	for ( cp = buf, ep = &cp[PWDSIZE-1] ; read(fd, &c, 1) == 1 ; )
	{
		if
		(
			(c &= 0177) == '\n'
			||
			c == '\r'	/* Special for Apollos */
			||
			c == '\0'
		)
		{
			*cp = '\0';
			writefd("\n", 1);
			echo_on();
			fd = SYSERROR;
			(void)signal(SIGINT, SIG_DFL);
			return;
		}

		if ( cp < ep )
			*cp++ = c;
	}

	echo_on();
	fd = SYSERROR;
	(void)signal(SIGINT, SIG_DFL);
	finish(EX_DATAERR);
}



void
printpasswd()
{
	Passwd	pw;

	(void)GetPassword(&pw, EmptyString);

	if ( PasswdData != NULLSTR )
		(void)fprintf(stdout, "%s", PasswdData);
}



void
setpasswd()
{
	char *	cp;
	char *	encrypted;
	bool	new;
	int	len;
	Passwd	pe;
	char	try1[PWDSIZE];
	char	try2[PWDSIZE];

	static char	fmt[]	= "%s:%s:%s\n";

	errno = 0;

	if ( GetPassword(&pe, PassName) && (!MatchedBC || strcmp(PassName, pe.P_name) == STREQUAL) )
	{
		new = false;
		Printf(english("%s password for \"%s\"\n"), Delete?"Deleting":"Changing", PassName);
	}
	else
	{
		if ( errno && errno != ENOENT )
			SysWarn(CouldNot, ReadStr, PasswdFile);

		if ( Delete )
		{
			Printf(english("No password for \"%s\"\n"), PassName);
			return;
		}

		new = true;
		Printf(english("Creating password for \"%s\"\n"), PassName);
	}

	if ( !Delete )
	{
		getpasswd(try1, english("New password"));
		getpasswd(try2, english("Retype new password"));

		if ( strcmp(try1, try2) != STREQUAL )
		{
			Printf(english("Mismatch - password unchanged.\n"));
			finish(EX_TEMPFAIL);
		}

		if ( try1[0] == '\0' )
			encrypted = try1;
		else
		{
			char *		save_ovc = ValidChars;

			ValidChars = CryptAlphabet;
			(void)EncodeNum(try2, (Ulong)(Pid+Time), -1);
			ValidChars = save_ovc;

			len = strlen(try2);
			encrypted = crypt(try1, try2+len-2);

			Trace4(1, "crypt(%s, %s) ==> %s", try1, try2+len-2, encrypted);
		}
	}

	(void)signal(SIGINT, SIG_IGN);

	LockFile = concat(PasswdFile, ".lock", NULLSTR);

	if ( !SetLock(LockFile, Pid, false) )
	{
		Recover(ert_exit);
		Error(CouldNot, LockStr, LockFile);
	}

	TmpFile = concat(PasswdFile, ".tmp", NULLSTR);

	fd = create(TmpFile);

	if ( new )
	{
		writefd(PasswdData, PasswdLength);

		if ( Comment == NULLSTR )
			Comment = EmptyString;

		len = strlen(fmt) + strlen(PassName) + strlen(encrypted) + strlen(Comment) + 1;
		cp = Malloc(len);
		(void)sprintf(cp, fmt, PassName, encrypted, Comment);

		writefd(cp, strlen(cp));
	}
	else
	if ( !Delete && Comment == NULLSTR && (len = strlen(encrypted)) == strlen(pe.P_crypt) )
	{
		strncpy(PasswdPoint, encrypted, len);

		writefd(PasswdData, PasswdLength);
	}
	else
	{
		if ( (len = PasswdStart - PasswdData) > 0 )
			writefd(PasswdData, len);

		if ( !Delete )
		{
			len = strlen(fmt) + strlen(PassName) + strlen(encrypted) + 1;

			if ( Comment != NULLSTR )
			{
				len += strlen(Comment);
				cp = Malloc(len);

				(void)sprintf
					(
						cp, fmt,
						PassName, encrypted, Comment
					);
			}
			else
			{
				len += PasswdEnd - PasswdTail;
				cp = Malloc(len);

				(void)sprintf
					(
						cp, "%s:%s:%.*s",
						PassName, encrypted,
						(int)(PasswdEnd-PasswdTail), PasswdTail
					);
			}

			writefd(cp, strlen(cp));
		}

		if ( (len = PasswdLength - (PasswdEnd - PasswdData)) > 0 )
			writefd(PasswdEnd, len);
	}

	(void)close(fd);

	cp = Malloc(strlen(PASSWDSORT) + strlen(TmpFile)*2);
	(void)sprintf(cp, PASSWDSORT, TmpFile, TmpFile);

	/*
	**	If you think I can be bothered doing a qsort on unstructured data,
	**	think again.
	*/

	(void)chmod(TmpFile, 0660);	/* `sort' may need group read/write permission */

	if ( system(cp) )
	{
		(void)SysWarn(CouldNot, "exec", cp);
		Printf(english("sort failed - please re-run as \"root\",\nor try changing PASSWDSORT - see \"man netpasswd\".\n"));
		finish(EX_TEMPFAIL);
	}

	(void)chmod(TmpFile, 0600);

	move(TmpFile, PasswdFile);

	(void)unlink(LockFile);
}



void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	echo_on();
	finish(EX_TEMPFAIL);
}



void
writefd(cp, n)
	char *	cp;
	int	n;
{
	if ( write(fd, cp, n) == SYSERROR )
	{
		Syserror(CouldNot, WriteStr, PasswdFile);
		finish(EX_OSERR);
	}
}
