/*
**	log	remote_tty
**
**	File exchange package for remote unices.
**
**	Also has transparent mode for talking to
**	other operating systems as if local tty.
**
**	For the following reasons this program must be
**	setuid to root for successful use:
**	1. The tty to be used for communication with other machine
**		is to be owned by root and is to have a mode of 600.
**	2. Needs to run at fairly high priority,
**		so nice will be used to obtain same.
**	3. The connect system call only allowed to be
**		used by root, to prevent abuse.
**
** Multiple opens of the same tty are assumed
**	to be taken care of by the appropriate tty
**	drivers (eg dz & dj) supporting the notion
**	of certain ttys being single open, and that
**	ttys used by 'log' for communications are
**	setup as single open.
*/

#define	WAIT_CALL
#define	STAT_CALL
#define	FILE_CONTROL
#define	SELECT
#define	SIGNALS
#define	STAT_CALL
#define	STDIO
#define	TERMIOCTL
#define	TIME
#define	TIMES
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"params.h"
#include	"Passwd.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#if	QNX
#undef	SYSVREL
#define	SYSVREL	3
#endif	/* QNX */

#if	SYSVREL >= 4
#include	<sys/stropts.h>
#endif	/* SYSVREL >= 4 */

#define SYSTEMID	Name
#define EOPENFAIL	-1

#define HIPRIORITY	-20
#define USERPRI		20
#define CONNECT		1		/* mode for ttyconnect system call */
/* (not DEBUGRECV)
#define DEBUGRECV	1	*/	/* print errors after receive stops */

/*
 * Regulate file transfer with a file protocol as follows:-
 *
 *	Blocks consist of a header followed by data followed by a trailer.
 *	The header contains a sequence number (0/1) and the size
 *	(transmitted twice, 2nd as complement of first).
 *	The trailer contains a longitudinal parity check of the data,
 *	followed by a sum check of the data.
 *
 *	Block format is
 *	STX SEQ SIZ ~SIZ ...DATA... LPC SUM
 *
 *	Blocks are acknowledged by a 2 byte reply consisting of
 *	ACK or NAK followed by the relevant sequence number.
 *
 * Note: that the DATAZ is really dependant on the accuracy
 *	of the crc check.
 */

#define REPLYZ		2
#define RTYPE		0
#define RSEQ		1

struct
{
	char r_typ;
	char r_seq;
}
reply;

#define HEADER		4
#define BSEQ		1
#define TRAILER		2
#define OVERHEAD	(HEADER+TRAILER)
#define DATAZ		64
#define DATAZL		64L
#define BUFZ		(DATAZ+OVERHEAD)

/*
 *	Timeouts - these must be tailored to the line speed,
 *	the basic formulae used being:-
 *	receive timeout = receive time + turnaround time
 *	transmit	"	= 2 * receive timeout
 *	(all times in seconds)
 */
#define STARTTIMEOUT	10
#define FLUSHTIMEOUT	1
#define TURNAROUND	4
#define XRETRYS		4
#define RRETRYS		5
#define GARBAGE		2	/* max garbage chars tolerated before reset */
#define EXTABR		1920	/* per BSD */
#define EXTBBR		3840	/* per BSD */
#define DEFAULTRATE	960	/* 9600 baud default */

short	byterate[16] =	/* Unix standard, buried in past */
{
	0,5,8,10,15,15,20,30,60,120,
	180,240,480,960,EXTABR,EXTBBR
};

unsigned	xtimeout, rtimeout;
long	xrate;			/* currently effective data transfer rate */
long	xtotal;			/* total bytes received/transmitted */
long	xtime;			/* total transmission time */

#define SOH		001	/* remote accept */
#define STX		002	/* start of block */
#define ETX		003	/* remote finish */
#define EOT		004	/* remote file access error */
#define ENQ		005	/* remote timeout */
#define ACK		006	/* xfer ok */
#define DLE		020	/* file already exists */
#define NAK		025	/* xfer failed */

#define MSOH		"\01\01",2
#define METX		"\03\03\03\03",4
#define MEOT		"\04\04",2
#define MTIMEOUT	"\05\05",2
#define MDLE		"\020\020",2

#if	SYSTEM < 3
struct sgttyb	save_tty[10];
#else	/* SYSTEM < 3 */
struct termio	save_tty[10];
#if	SYSVREL >= 4
typedef struct ld_s	ld_s;
struct ld_s
{
	ld_s *	next;
	char *	name;
} *		save_ld[10];
#endif	/* SYSVREL >= 4 */
#endif	/* SYSTEM < 3 */
bool	EvenP;
bool	OddP;
bool	Par0;
bool	Par1;
bool	RawSet[10];		/* One for each save_tty */
short	narg;
char	*Name;			/* name by which log is invoked */
Passwd	Invoker;		/* Person invoked by */
char	*cmd[10];
short	xfd	= 2;		/* fd for remote port */
char	otherm[64] =	"/dev/";
struct stat	ostatb;
char	c;
int	gid;
int	uid;
long	filesize;

char exists[] = "%s already exists - are you sure? ";

/* defines for use with access calls */
#define R	04	/* read permission */
#define W	02	/* write permission */
#define X	01	/* execute / directory search permission */


#define ESCAPE	031	/* default escape char is ctrl/y */
#define ESCAPED "^y"	/* default escape description */
char	quit;		/* escape code */
char	*quitdescrpt;	/* escape code description */

#ifndef	B19200
#ifdef	EXTA
#define		B19200		EXTA
#endif	/* EXTA */
#endif	/* !B19200 */

#ifndef	B38400
#ifdef	EXTB
#define		B38400		EXTB
#endif	/* EXTB */
#endif	/* !B38400 */

struct
{
	char *	name;
	char *	descpt;	/* description of control code that follows */
	char	escape;	/* default escape code for this line */
	char	speed;	/* sets log line to this speed if non-zero */
}
	quits[] =
{
	{"log300",	ESCAPED,	ESCAPE,	B300},
	{"log1200",	ESCAPED,	ESCAPE,	B1200},
	{"log2400",	ESCAPED,	ESCAPE,	B2400},
	{"log4800",	ESCAPED,	ESCAPE,	B4800},
	{"log9600",	ESCAPED,	ESCAPE,	B9600},
#	ifdef	B19200
	{"log19200",	ESCAPED,	ESCAPE,	B19200},
#	endif	/* B19200 */
#	ifdef	B38400
	{"log38400",	ESCAPED,	ESCAPE,	B38400},
#	endif	/* B38400 */
#	ifdef	EXTA
	{"logEXTA",	ESCAPED,	ESCAPE,	EXTA},
#	endif	/* EXTA */
#	ifdef	EXTB
	{"logEXTB",	ESCAPED,	ESCAPE,	EXTB},
#	endif	/* EXTB */
		{0,0,0,0}
};

struct
{
	char *	name;
	int	speed;
}
	speeds[] =
{
	{"300",		B300},
	{"1200",	B1200},
	{"2400",	B2400},
	{"4800",	B4800},
	{"9600",	B9600},
#	ifdef	B19200
	{"19200",	B19200},
#	endif	/* B19200 */
#	ifdef	B38400
	{"38400",	B38400},
#	endif	/* B38400 */
#	ifdef	EXTA
	{"EXTA",	EXTA},
#	endif	/* EXTA */
#	ifdef	EXTB
	{"EXTB",	EXTB},
#	endif	/* EXTB */
	{"2400",	0},	/* Default */
		{0,0}
};

char *	Speed	= "9600";

struct err
{
	short	e_count;
	char	*e_mess;
}
	errs[] =
{
#define E_RETRYS	0
	{ 0,	"retrys" },
#define E_SEQUENCE	1
	{ 0,	"seq" },
#define E_SIZE		2
	{ 0,	"size" },
#define E_OUTSIZE	3
	{ 0,	"outsize" },
#define E_TIMEOUTS	4
	{ 0,	"timeouts" },
#define E_CRC		5
	{ 0,	"crc" },
#define E_WRITE		6
	{ 0,	"local write" },
#define E_NAKS		7
	{ 0,	"naks" },
#define E_SYNC		8
	{ 0,	"sync" },
#define E_READ		9
	{ 0,	"local read" }
};
#define NERRS		((sizeof errs)/(sizeof errs[0]))

char	buf[BUFZ];

/*
**	Configurable parameters.
*/

int	Traceflag;
int	UUCPlocked;
char *	UUCPlockfile;
int	UUCPlowerdev	= UUCPLOWERDEV;
int	UUCPmlockdev	= UUCPMLOCKDEV;
int	UUCPstrpid	= UUCPSTRPID;

ParTb	Params[] =
{
	{"TRACEFLAG",	(char **)&Traceflag,	PINT},
	{"UUCPLCKDIR",	&UUCPLCKDIR,		PDIR},
	{"UUCPLCKPRE",	&UUCPLCKPRE,		PSTRING},
	{"UUCPLOCKS",	(char **)&UUCPLOCKS,	PINT},
	{"UUCPLOWERDEV",(char **)&UUCPlowerdev,	PINT},
	{"UUCPMLOCKDEV",(char **)&UUCPmlockdev,	PINT},
	{"UUCPSTRPID",	(char **)&UUCPstrpid,	PINT},
};

extern void	evenp _FA_((void));
extern void	mylog _FA_((int, char **));
extern void	nopar _FA_((void));
extern void	oddp _FA_((void));
extern bool	okuse _FA_((void));
extern int	opentty _FA_((char *));
extern void	par0 _FA_((void));
extern void	par1 _FA_((void));
extern void	reset_raw _FA_((int));
extern void	sbreak _FA_((int));
extern SigFunc	sentinel;
extern int	set_raw _FA_((int, int));
extern int	Sync _FA_((char *));

extern void	rmlock _FA_((char *));
extern int	mlock _FA_((char *));

int	accreat(char *, int);
void	errors(void);
void	fini_remote(int);
char *	fquit(void);
void	get(void);
int	_getline(void);
void	help(void);
void	init_remote(int, int);
int	lflush(int);
void	logconnect(void);
int	logsend(int, char**);
int	mycrc(int);
void	put(void);
int	receive(int, char**);
int	rproto(int, int);
void	transfer(char *, char *, char *);
void	setspeed(int, char *);
void	setrate(long);
void	sh(void);
int	Sync(char *);
void	tty(void);
int	xproto(int, int);

/****************************************************************/

int
main(argc,argv)
	int	argc;
	char *	argv[];
{
	int	ret = EX_USAGE;

#	ifndef	QNX
	nice(HIPRIORITY);
#	endif	/* QNX */
	gid = getgid();
	uid = getuid();

	InitParams();
	GetParams("daemon", Params, sizeof Params);
	GetParams("netcon", Params, sizeof Params);

	GetInvoker(&Invoker);

	sentinel(0);	/* catch all real-time limit expiries */

	if ( (Name = strrchr(argv[0], '/')) == NULLSTR )
		Name = argv[0];
	else
		Name++;

	switch ( *Name )
	{
	case 'S':
	case 's':
		ret = logsend(argc,argv);
#		ifdef	DEBUGSEND
		errors();
#		endif	/* DEBUGSEND */
		break;

	case 'R':
	case 'r':
		ret = receive(argc,argv);
#		ifdef	DEBUGRECV
		errors();
#		endif	/* DEBUGRECV */
		break;

	default:
		if ( okuse() )
		{
			mylog(argc, argv);
			errors();	/* tell about the errors encountered */
			ret = EX_OK;
		}
		else
			(void)printf("Illegal usage!\n");
		break;
	}

	exit(ret);
	return ret;	/* To keep gcc happy (it doesn't like main being declared "void") */
}

/*****************************************************************/

void
mylog(argc, argv)
	int		argc;
	char **		argv;
{
	register char *	otp;
	register int	i;
	char *		arg;
	char *		spd = NULLSTR;

	if ( argc < 2 || argc > 3 )
	{
		printf("Usage: log remote-special-file [speed]\n");
		exit(1);
	}

	if ( argv[1][0] == '/' )
		(void)strncpy(&otherm[0], argv[1], 19);
	else
		(void)strncpy(&otherm[5], argv[1], 14);	/* Prepend "/dev/" */

	if ( access(otherm, 02) == SYSERROR && !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	(void)stat(otherm, &ostatb);

	if ( (ostatb.st_mode & 0777) == 0000 )
	{
		Warn(english("Device \"%s\" mode 0."), otherm);
		exit(EX_TEMPFAIL);
	}

	(void)chmod(otherm, 0000);	/* protect against other access */
	(void)chown(otherm, 0, 0);	/* protect against other access */

	if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGINT, finish);
	(void)signal(SIGTERM, finish);

	if ( UUCPLOCKS )
	{
		if ( geteuid() )
		{
			printf("Must run as root.\n");
			finish(2);
		}

		if ( UUCPlocked )
		{
			rmlock(NULLSTR);
			UUCPlocked = 0;
		}

		if ( strncmp(otherm, DevNull, 5) == STREQUAL )
			otp = otherm+5;
		else
			otp = otherm;

		if ( mlock(otp) != 0 )
		{
			printf("%s locked (UUCP)\n", otp);
			finish(2);
		}

		UUCPlocked = 1;
	}
	else
		UUCPlocked = 0;

	if ( (xfd = opentty(otherm)) == SYSERROR )
	{
		perror(otherm);
		finish(1);
	}

	if ( argc == 3 )
		spd = argv[2];

	arg = argv[1];
	quit = ESCAPE;
	quitdescrpt = ESCAPED;
	for ( i = 0 ; quits[i].name ; i++ )
	{
		otp = quits[i].name;
		if ( strcmp(arg, otp) == 0 )
		{
			quit = quits[i].escape;
			quitdescrpt = quits[i].descpt;
			break;
		}
	}
	if ( spd != NULLSTR )
		setspeed(xfd, spd);
	else
	if ( quits[i].name == NULLSTR ) /* not in table */
		init_remote(xfd, 0);
	else
		init_remote(xfd, quits[i].speed);

	tty();

	for ( ;; )
	{
		errors();
		printf("* ");

		switch ( _getline() )
		{
		case '0':	par0();		/* Switch to 0 parity */
				tty();		/* and back to remote */
				break;

		case '1':	par1();		/* Switch to 1 parity */
				tty();		/* and back to remote */
				break;

		case 'b':	sbreak(xfd);	/* send BREAK */
				tty();		/* and back to remote */
				break;

		case 'c':	if ( narg == 2 )
				{
					if (access(cmd[1], X) ||
						chdir(cmd[1]) == -1 ) perror(cmd[1]);
					break;
				}

		case 'e':	evenp();	/* Switch to EVENP */
				tty();		/* and back to remote */
				break;

		case 'g':	if (narg == 3)
					get();	/* get a file from remote */
				break;

		case 'r':
		case 'n':	nopar();	/* Switch off parity */
				tty();		/* and back to remote */
				break;

		case 'o':	oddp();		/* Switch to ODDP */
				tty();		/* and back to remote */
				break;

		case 'p':	if (narg == 3)
					put();	/* put a file to remote */
				break;

		case 'q':	if (narg == 2)
					quitdescrpt = fquit();	/* change quit code */
				break;

		case 's':	if (narg == 2)
				{
					setspeed(xfd, cmd[1]);
					tty();		/* and back to remote */
				}
				else
					help();
				break;

		case 't':	if (narg == 1)
					tty(); /* talk to remote as a terminal */
				break;

		case 'x':	if (narg == 1)
				{
					finish(0); /* eXit */
					return;
				}
				break;

		case '!':	sh();		/* local escape to shell */
				break;

		default:	help();			/* unknown command */
				break;
		}
	}
}

/**************/
int
_getline()
{
	static char	line[100];
	register char	*q;
	register char	**p = (char **)0;	/* gcc -Wall */
	register int	toggle;

	fflush(stdout);
	if ((toggle = read(2,line,sizeof line)) == 1) return(-1);
	if (toggle <= 0) finish(0);
	if (line[toggle - 1] != '\n')
	{
		printf("Too verbose!!\n");
		do
			toggle = read(2, line, sizeof line);
		while (toggle > 0 && line[toggle - 1] != '\n');
		return -1;
	}
	for (q = line; q < &line[sizeof line] && *q == ' '; q++);
	if (*q == '!')
	{
		cmd[0] = q;
		narg = 1;
		q = &line[toggle - 1];
	}
	else
	{
		p = cmd; narg = 0; toggle = 1;
		for ( ; *q != '\n'; q++)
			if (*q == ' ')
			{
				toggle = 1;
				*q = 0;
			}
			else if (toggle)
			{
				*p++ = q;
				narg++;
				toggle = 0;
			}
	}
	*q = 0;
	q = cmd[0];
	if ((narg == 2) && ((*q == 'p') || (*q == 'g')))
	{
		*p++ = cmd[1];
		*p = 0;
		narg++;
	}
	if (narg == 0) return -1;
	return *q;
}

/*******************/

void
fini_remote(fd)
	int	fd;
{
	if ( RawSet[fd] )
		reset_raw(fd);
}

void
setspeed(fd, s)
	int		fd;
	char *		s;
{
	register int	i;

	for ( i = 0 ; speeds[i].name ; i++ )
		if ( strcmp(s, speeds[i].name) == 0 )
		{
			init_remote(fd, speeds[i].speed);
			break;
		}

	if ( speeds[i].name == (char *)0 )
		printf("invalid speed \"%s\"\n", s);
}

void
init_remote(fd, spd)
	int		fd;
	int		spd;
{
	register int	speed;

	spd = set_raw(fd, spd);

	for ( speed = 0 ; speeds[speed].name ; speed++ )
		if ( spd == speeds[speed].speed )
			Speed = speeds[speed].name;

	if ( (speed = byterate[spd & 017]) == 0 )
		speed = DEFAULTRATE;
	xtimeout = 2 * (rtimeout = BUFZ / speed + TURNAROUND);
	xrate = (speed * DATAZL) / BUFZ;
}


void
set_rate(t)
long t;
{
	xtotal += filesize;
	xtime += t;
	xrate = xtotal/xtime;
}

/*********************/
void
finish(err)
	int	err;
{
	if ( UUCPlocked )
	{
		rmlock(NULLSTR);
		UUCPlocked = 0;
	}
	fini_remote(xfd);
	(void)chown(otherm, ostatb.st_uid, ostatb.st_gid);
	(void)chmod(otherm, ostatb.st_mode & ~S_IFMT);
	exit(err);
}

/*************************************************************/

char *
fquit()
{
	static char	qs[3];
	register char	*qp = qs;

	if ((quit = *cmd[1] & 0177) == 037) return("US");	/* clear-screen char. */
	if (quit < 040)
	{
		*qp++ = '^';
		*qp++ = quit + 0140;
	}
	else *qp++ = quit;
	*qp = 0;
	return(qs);
}

/*************************************************************/

void
sh()
{
	char *args[4];
	register char **ap = args;
	register int i = 0;
	static char shf[] = "/bin/sh";

	cmd[0]++;
	*ap++ = shf+5;
	*ap++ = "-c";
	*ap++ = cmd[0];
	*ap = 0;

	if (!(i = fork()))
	{
		close(xfd);
#		ifndef	QNX
		nice(USERPRI);
#		endif	/* QNX */
		setgid(gid);
		setuid(uid);
		execv(shf, args);
		perror(shf);
		exit(-1);
	}
	if (i > 0)
	{
		int	wv;
		void 	(*handler)();

		handler = (void (*)())signal(SIGINT, SIG_IGN);
		wait(&wv);
		(void)signal(SIGINT, handler);
	}
	else perror("try again");
}

/*************************************************************/

void
tty()
{
	/*
	 * logically connect local terminal to remote until quit char received
	 */
	printf("\t%s -> %s @ %s baud ('%s'=quit)\n", SYSTEMID, otherm, Speed, quitdescrpt);
/*
	if ( ttyconnect(2, xfd, CONNECT, quit) == SYSERROR )
	{
		perror("connect failed");
		finish(1);
	}
*/
	logconnect();
}

void
logconnect()
{
	int	pid;
	char	c;

	(void)set_raw(2, 0);

	switch ( pid = fork() )
	{
	case -1:	perror("connect");
			reset_raw(2);
			finish(1);
			break;
	case 0:		for ( ;; )
			{
				while ( read(xfd, &c, 1) != 1 )
					(void)sleep(1);

				c &= 0177;

				(void)write(2, &c, 1);
			}
			break;
	default:	while ( read(2, &c, 1) == 1 )
			{
				c &= 0177;

				if ( c == quit )
					break;

				if ( Par1 )
					c |= 0200;

				(void)write(xfd, &c, 1);
			}
	}

	(void)kill(pid, 9);
	while ( wait((int *)0) != pid )
		;
	reset_raw(2);
}

/*************************************************************/

void
get()
{
	register int	ofd;
	register long	t;

	if ((ofd = accreat(cmd[2], 0)) == SYSERROR) return;

	transfer("/usr/lib/Send",cmd[1], (char *)0);

	if (Sync("open"))
	{
		printf(" -- receive rate approx. %lu bytes/sec.\n", (PUlong)xrate);
		t = time((time_t *)0);
		if (rproto(ofd, xfd) == 0)
		{
			if (!(t = time((time_t *)0) - t))	t++;
			set_rate(t);
			printf("%s: %lu bytes received in %lu secs (%lu bytes/sec)\n", cmd[2], (PUlong)filesize, (PUlong)t, (PUlong)(filesize/t));
		}
		else printf(" -- transfer aborted after %lu bytes\n", (PUlong)filesize);
	}

	lflush(xfd);
	close(ofd);
}

/*************************************************************/

void
put()
{
	register int	ifd;
	register int	n;
	register int	fred;
	long		t;
	struct stat	statb;

	if (access(cmd[1],R) || ((ifd = open(cmd[1], O_READ)) == SYSERROR))
	{
		perror(cmd[1]);
		return;
	}

	fstat(ifd, &statb);
	filesize = statb.st_size;

	transfer("/usr/lib/Receive",cmd[2], (char *)0);

	if ((n = Sync("create")))
	{
		if (n < 0)
		{
			printf(exists, cmd[2]);
			fred = 0;
			if ((n = read(2, &c, 1)) == 1 && c == 'y') fred++;
			if (n == 1 && c != '\n')
			do
				n = read(2, &c, 1);
			while (n > 0 && c != '\n');
			if (fred == 0) return;
			lflush(xfd);
			transfer("/usr/lib/Receive", cmd[2], "-");
			if (Sync("create") != 1) goto home;
		}

		printf(" -- estimated transmit time: %lu secs\n", (PUlong)(filesize/xrate));
		t = time((time_t *)0);

		if ((n = xproto(ifd, xfd)) == 0)
		{
			if (!(t = time((time_t *)0) - t))	t++;
			set_rate(t);
			printf("%s: %lu bytes transmitted in %lu secs (%lu bytes/sec)\n", cmd[2], (PUlong)filesize, (PUlong)t, (PUlong)filesize/t);
		}
		else printf(" --%stransfer aborted after %lu bytes\n", (n == 2) ? " remote terminate -- " : " ", (PUlong)filesize);
	}

home:
	lflush(xfd);
	close (ifd);
}

/*******************************/

void
transfer(s1, s2, s3)
char	*s1, *s2, *s3;
{
	register char	*p, *q;

	q = buf;

	*q++ = '@';
	*q++ = '@';	/* two will ALWAYS do the trick! */

	for ( p = s1 ; (*q++ = *p++) ; );
	q[-1] = ' ';
	for ( p = s2 ; (*q++ = *p++) ; );
	if (s3 != (char *)0)
	{
		q[-1] = ' ';
		for ( p = s3 ; (*q++ = *p++) ; );
	}
	q[-1] = '\n';

	write (xfd, buf, q-buf);
}


/*******************************/

int
logsend(argc,argv)
	int		argc;
	char **		argv;
{
	register int	fd;
	register int	ret = EX_USAGE;

	init_remote(2, 0);

	if (argc != 2 || access(argv[1],R) || ((fd = open(argv[1], O_READ))<0)) write(2, MEOT);
	else
	{
		write(2, MSOH);
		sleep(FLUSHTIMEOUT);
		ret = xproto(fd, 2);
	}

	lflush(2);
	fini_remote(2);
	return ret;
}

/*******************************/

int
receive(argc,argv)
	int		argc;
	char **		argv;
{
	register int	fd;
	register int	ret = EX_USAGE;

	init_remote(2, 0);

	if (argc > 3 || (fd = accreat(argv[1], argc == 3 ? 2 : 1)) == SYSERROR) write(2, MEOT);
	else
	{
		write(2, MSOH);
		ret = rproto(fd, 2);
	}

	lflush(2);
	fini_remote(2);
	return ret;
}

/********************************************************************/

short	timedout;

void
sentinel(sig)
	int	sig;
{
	(void)signal(SIGALRM, sentinel);
	timedout++;
}

int
tread(f, b, s, timeout)
	int		f;
	register char * b;
	register int	s;
	unsigned int	timeout;
{
	register int	error = 0;
	register int	x;

	alarm(timeout);
	timedout = 0;
	while ( (x = read(f, b, s)) != s )
		if ( x != SYSERROR )
		{
			b += x;
			s -= x;
		}
		else
		{
			error++;
			break;
		}
	alarm(0);
	return(timedout||error);
}

int
lflush(fd)
	int fd;
{
	while (!tread(fd, &c, 1, FLUSHTIMEOUT));
	return(1);
}

/********************************************************************/

int
Sync(remote_err)
char	*remote_err;
{
	do
	{
		if (tread(xfd, &c, 1, STARTTIMEOUT))
		{
			printf("no response from remote!\n");
			return(0);
		}
		if (c == EOT)
		{
			printf("can't %s remote file!\n", remote_err);
			return(0);
		}
		if (c == DLE) return -1;
	} while (c != SOH);

	lflush(xfd);
	return(1);
}

/******************************************************************/

int
xproto(local, remote)
	int local, remote;
{
	register int	n, retrys, synce;

	buf[BSEQ] = 1;
	filesize = 0;

	while ((n = read(local, buf+HEADER, DATAZ)) >= 0)
	{
		mycrc(n);
		retrys = 0;

		do
		{
			write(remote, buf, n+OVERHEAD);

			synce = 0;
			while (tread(remote, (char *)&reply, 1, xtimeout) == 0)
			{
				switch (reply.r_typ)
				{
				default:
					errs[E_SYNC].e_count++;
					if (++synce > GARBAGE)
						break;
					else
						continue;

				case ENQ:
				case ACK:
				case NAK:
				case ETX:
					tread(remote, (char *)&reply.r_seq, 1, xtimeout);
				}
				break;
			}

			if (reply.r_typ == ETX && reply.r_seq == ETX)
				return(n ? EX_PROTOCOL : EX_REMTERM);	/* remote finish */

		} while (

				( (synce > GARBAGE && lflush(remote))
					|| (timedout && ++errs[E_TIMEOUTS].e_count)
					|| (reply.r_typ != ACK && ++errs[E_NAKS].e_count)
					|| (reply.r_seq != buf[BSEQ] && ++errs[E_SEQUENCE].e_count)

				)
				&& retrys++ < XRETRYS
			);

		errs[E_RETRYS].e_count += retrys;
		if (retrys > XRETRYS)
		{
			errs[E_RETRYS].e_count--;
			return(EX_PROTOCOL);
		}

		if (n == 0)	/* eof */
			return(EX_OK);

		filesize += n;
	}

	errs[E_READ].e_count++;

	return(EX_PROTOCOL);
}

/*******************************************************************/

/* case labels */
#define NEWBLOCK	0
#define SEQUENCE	1
#define SIZE1		2
#define SIZE2		3

int
rproto(local, remote)
	int local, remote;
{
	register int	size, state, x;
	register int	wretrys, lastsize, retrys, garbage;
	char		lastseq;

	state = NEWBLOCK;
	retrys = 0;
	reply.r_seq = 0;
	filesize = 0;
	lastseq = 0xff;
	garbage = 0;
	size = lastsize = 0;

	for (;;)
	{

		while (tread(remote, &c, 1, rtimeout))
			if (retrys++ < RRETRYS)
			{
				errs[E_TIMEOUTS].e_count++;
				write(remote, MTIMEOUT);
			}
			else
			{
	fail:
				write(remote, METX);
				return(EX_PROTOCOL);
			}

		x = c & 0377;

		switch(state)
		{
		case NEWBLOCK:
			if (x == STX)
			{
				state = SEQUENCE;
				retrys = 0;
				garbage = 0;
			}
			else
			{
				if (garbage++ >= GARBAGE)
				{
					lflush(remote);
					garbage = 0;
				}
			}
			break;

		case SEQUENCE:
			if (x & ~1)
			{
				errs[E_SEQUENCE].e_count++;
fnak:
				state = NEWBLOCK;
				lflush(remote);
nak:
				reply.r_typ = NAK;
				errs[E_NAKS].e_count++;
rply:
				write(remote, (char *)&reply, REPLYZ);
				break;
			}
			reply.r_seq = x;
			state = SIZE1;
			break;

		case SIZE1:
			if ((size = x) > DATAZ)
			{
				errs[E_OUTSIZE].e_count++;
				goto fnak;
			}
			state = SIZE2;
			break;

		case SIZE2:
			state = NEWBLOCK;
			if (size != (~x & 0377))
			{
				errs[E_SIZE].e_count++;

				goto fnak;
			}
			if (size == 0)	/* eof */
			{
				write(remote, METX);
				return(EX_OK);
			}

			if ((tread(remote, buf+HEADER, size+TRAILER, rtimeout) && ++errs[E_TIMEOUTS].e_count)
			|| (mycrc(size) && ++errs[E_CRC].e_count)
			)
				goto nak;

			if (reply.r_seq == lastseq && lastsize > 0)
			{
				lseek(local, -(long)lastsize, 1);
				filesize -= lastsize;
			}

			wretrys = 0;
			while ((lastsize = write(local, buf+HEADER, size)) != size)
			{
				errs[E_WRITE].e_count++;
				if (wretrys++ >= XRETRYS)
					goto fail;
				if (lastsize > 0)
					lseek(local, -(long)lastsize, 1);
			}
			filesize += size;

			reply.r_typ = ACK;
			lastseq = reply.r_seq;
			goto rply;
		}
	}
}

/********************************************************************/

/*
 *	not a true crc, but nearly so for small 's' (< 100 say)
 */

int
mycrc(s)
	int		s;
{
	register char *	p;
	register char	lpc, sum;
	register char	*end;
	register int	error;

	p = buf;	lpc = 0;	sum = 0;	error = 0;

	*p++ = STX;
	*p++ ^= 1;		/* flip sequence number */
	*p++ = s;	*p++ = ~s;

	end = buf+HEADER+s;
	for ( ; p < end; p++)
	{
		lpc ^= *p;
		sum += *p;
	}

	if (lpc != *p)	error++;
	*p++ = lpc;
	if (sum != *p)	error++;
	*p++ = sum;

	return(error);
}

/**********************************************************************/

void
errors()
{
	register struct err *ep;
	register int nerrs;

	nerrs = 0;

	for (ep = errs; ep < &errs[NERRS]; ep++)
	if (ep->e_count)
	{
		if (nerrs++ == 0)
			printf(" file transfer protocol errors:-\n");
		printf(" %s: %d", ep->e_mess, ep->e_count);
		ep->e_count = 0;
	}

	if (nerrs)	printf("\n");
}

/***********************************************************************/

void
help()
{
	printf("\nuse:\n");
	printf("\t!{shell command} - execute shell command\n");
	printf("\t0 - set 0 parity and go back to remote\n");
	printf("\t1 - set 1 parity and go back to remote\n");
	printf("\tb[reak] - send BREAK and go back to remote\n");
	printf("\tc[d] directory - change directory\n");
	printf("\te[venp] - set EVEN parity and go back to remote\n");
	printf("\tg[et] %s-file [%s-file] - get file\n", &otherm[5], SYSTEMID);
	printf("\tn[oparity] - set RAW mode (NO parity) and go back to remote\n");
	printf("\to[ddp] - set ODD parity and go back to remote\n");
	printf("\tp[ut] %s-file [%s-file] - put file\n", SYSTEMID, &otherm[5]);
	printf("\tq[uit] new-code - set new quit char\n");
	printf("\tr[aw] - set RAW mode and go back to remote\n");
	printf("\ts[peed] line-speed - set line speed\n");
	printf("\tt[ty] - back to remote\n");
	printf("\tx - exit\n\n");
}

/*******************************************************************/

#ifdef	SPRINTF
#define		NUMBERS
#define		LONG
#include	<sprintf.h>
#endif	/* SPRINTF */

/*
**	check for appropriate permissions on a file
**	passed as 'arg' and creat it if allowable.
*/

int
accreat(arg, flag)
	char *arg;
	int flag;
{
	register char *p;
	register int m, n;
	extern int errno;

	m = 0;
	if ((n = access(arg, W)) == 0)		/* file already exists and have write permission */
	{
		if (flag == 0)
		{
			printf(exists, arg);
			if (_getline() != 'y') return -1;
		}
		else if (flag == 1)
		{
			write(2, MDLE);
			return -1;
		}
	}
	else if (n && errno == ENOENT)
	{				/* isolate directory name and check it */
		p = arg;
		while (*++p);
		while (*--p != '/' && p > arg);
		if (*p != '/') n = access(".", W|X);
		else if (p == arg) n = access("/", W|X);
		else
		{
			*p = 0;
			n = access(arg, W|X);
			*p = '/';
		}
		m++;
	}
	if (n == 0)		/* creat the file since access is granted */
		n = creat(arg, 0600);
	if (n == SYSERROR)
		perror(arg);
	if ( m )
		chown(arg, uid, gid);
	return n;
}

/*
 *	validate that log is not being used with any
 *	standard input or output redirection - this
 *	includes asynchronous use.
 *	The test checks that file descriptors 0, 1, and 2 represent
 *	the same tty.
 */

bool
okuse()
{
	register int	i;
	struct stat	sbuf[3];

	for ( i = 0 ; i < 3 ; i++ )
		if
		(
			fstat(i, &sbuf[i]) == SYSERROR
			||
			(sbuf[i].st_mode & S_IFMT) != S_IFCHR	/* not a tty */
			||
			(
				i	/* some sort of redirection? */
				&&
				(
					sbuf[i].st_dev != sbuf[i-1].st_dev
					||
					sbuf[i].st_ino != sbuf[i-1].st_ino
				)
			)
		)
			return false;

	return true;
}


#if	V8
extern int	tty_ld;
extern int	ntty_ld;
#endif	/* V8 */



/*
**	Re-set saved mode on 'tty' link.
*/

void
reset_raw(fd)
	int	fd;
{
#	if	SYSTEM < 3

	if ( stty(fd, &save_tty[fd]) == SYSERROR )
		perror("stty");

#	else	/* SYSTEM < 3 */

#	if	SYSVREL >= 4
	ld_s *	p;

	while ( (p = save_ld[fd]) != (ld_s *)0 )
	{
		save_ld[fd] = p->next;
		Trace2(1, "pushing streams module %s\r", p->name);
		if ( ioctl(fd, I_PUSH, p->name) == SYSERROR )
			(void)SysWarn("push stream module %s\r", p->name);
		free((char *)p);
	}
#	endif	/* SYSVREL >= 4 */

	if ( ioctl(fd, TCSETA, &save_tty[fd]) == SYSERROR )
		perror("ioctl(TCSETA)");

#	endif	/* SYSTEM < 3 */

	RawSet[fd] = false;
}



/*
**	Set RAW mode on 'tty' link
*/

int
set_raw(fd, speed)
	int	fd;
	int	speed;
{
#	if	SYSTEM < 3
	struct sgttyb	params;
#	ifdef	FIOLOOKLD
	int		ld;
#	endif	/* FIOLOOKLD */

	if ( gtty(fd, &params) == SYSERROR )
	{
		perror("gtty");
		return 0;
	}

	if ( fd > 9 )
	{
		printf("fd too large: %d\n", fd);
		exit(1);
	}

	save_tty[fd] = params;

	if ( speed )
		params.sg_ispeed = params.sg_ospeed = speed;
	else
		speed = params.sg_ispeed;

	if ( EvenP || OddP )
	{
		params.sg_flags = CBREAK|(EvenP?EVENP:ODDP);
	}
	else
		params.sg_flags = RAW;

#	ifdef	FIOLOOKLD
	if
	(
		(minchars = ioctl(fd, FIOLOOKLD, (struct sgttyb *)0)) == tty_ld
		||
		minchars == ntty_ld
	)
		(void)ioctl(fd, FIOPOPLD, (struct sgttyb *)0);
#	endif	/* FIOLOOKLD */

	if ( stty(fd, &params) == SYSERROR )
		perror("stty");

#	else	/* SYSTEM < 3 */

#	if	SYSVREL >= 4
	char	lookbuf[FMNAMESZ+1];
#	endif	/* SYSVREL >= 4 */
	static struct termio	params;

	if ( ioctl(fd, TCGETA, &params) == SYSERROR )
	{
		perror("ioctl(TCGETA)");
		return 0;
	}

	if ( fd > 9 )
	{
		printf("fd too large: %d\n", fd);
		exit(1);
	}

	save_tty[fd] = params;

	params.c_iflag &= ~(ISTRIP|INLCR|ICRNL|IGNCR|IXON|IXOFF|INPCK|BRKINT|PARMRK
#	ifdef	IUCLC
			|IUCLC
#	endif	/* IUCLC */
#	ifdef	IXANY
			|IXANY
#	endif	/* IXANY */
			);
	params.c_iflag |= IGNBRK|IGNPAR;
	if ( OddP || EvenP )
		params.c_iflag |= ISTRIP;

	params.c_oflag &= ~OPOST;

	if ( speed == 0 )
		speed = params.c_cflag & CBAUD;

	if ( OddP || EvenP )
	{
		params.c_cflag &= ~CBAUD;
		params.c_cflag |= speed|CREAD|CS8|CLOCAL|PARENB|(OddP?PARODD:0);
	}
	else
	{
		params.c_cflag &= ~(PARENB|CBAUD);
		params.c_cflag |= speed|CREAD|CS8|CLOCAL;
	}

	params.c_lflag = 0;

	params.c_cc[VMIN] = 1;
	params.c_cc[VTIME] = 0;

	if ( ioctl(fd, TCSETAF, &params) == SYSERROR )
		perror("ioctl(TCSETAF)");

#	if	SYSVREL >= 4
	while ( ioctl(fd, I_LOOK, lookbuf) != SYSERROR )
	{
		char *	s;
		ld_s *	p;

		Trace2(2, "found streams module %s\r", lookbuf);
		if
		(
			strcmp(lookbuf, (s = "ttcompat")) != 0
			&&
			strcmp(lookbuf, (s = "ldterm")) != 0
		)
			break;
		Trace2(1, "popping streams module %s\r", lookbuf);
		if ( ioctl(fd, I_POP, 0) == SYSERROR )
		{
			(void)SysWarn("pop stream module %s\r", lookbuf);
			break;
		}
		p = Talloc(ld_s);
		p->next = save_ld[fd];
		save_ld[fd] = p;
		p->name = s;
	}
#	endif	/* SYSVREL >= 4 */
#	endif	/* SYSTEM < 3 */

	RawSet[fd] = true;

	return speed;
}



/*
**	Set EVENP mode on 'tty' link
*/
void
evenp()
{
	EvenP = true;
	OddP = Par0 = Par1 = false;
}



/*
**	Set ODDP mode on 'tty' link
*/
void
oddp()
{
	OddP = true;
	EvenP = Par0 = Par1 = false;
}



/*
**	Turn off parity mode on 'tty' link
*/
void
nopar()
{
	EvenP = OddP = Par0 = Par1 = false;
}



/*
**	Set 0 parity mode on 'tty' link
*/
void
par0()
{
	Par0 = true;
	EvenP = OddP = Par1 = false;
}



/*
**	Set 1 parity mode on 'tty' link
*/
void
par1()
{
	Par1 = true;
	EvenP = OddP = Par0 = false;
}



/*
**	Send a BREAK to remote.
*/

void
sbreak(fd)
	int	fd;
{
#	if	SYSTEM >= 3
	if ( ioctl(fd, TCSBRK, 0) == SYSERROR )
		perror("ioctl(TCSBRK)");
#	endif	/* SYSTEM >= 3 */

#	if	BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c')
	if ( ioctl(fd, TIOCSBRK, (char *)0) == SYSERROR )
		perror("ioctl(TIOCSBRK)");

	(void)sleep(MINSLEEP);	/* Big break */

	if ( ioctl(fd, TIOCCBRK, (char *)0) == SYSERROR )
		perror("ioctl(TIOCCBRK)");
#	endif	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */
}



/*
**	``open'' for ``tty'' type circuits.
*/

#if	SYSTEM >= 3
int
opentty(name)
	char *		name;
{
	register int	fd;

	alarm(20);
	if ( (fd = open(name, O_NDELAY|O_RDWR)) == SYSERROR )
		return fd;
	alarm(0);

	set_raw(fd, 0);	/* CLOCAL */

#	if	CARR_ON_BUG == 1
	/*
	**	This fixes a bug on some SYSTEM xx. Driver sets CARR_ON on open
	**	or on modem DCD becoming high but not on the above ioctl!
	*/

	(void)close(open(name, O_RDWR, 0));
#	endif	/* CARR_ON_BUG == 1 */

	if ( fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NDELAY) == SYSERROR )
	{
		(void)close(fd);
		return SYSERROR;
	}

	return fd;
}
#else	/* SYSTEM >= 3 */



#if	BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c')
int
opentty(name)
	char *		name;
{
	register int	fd;
#	ifdef	LNOMDM
	int		lflag = LNOHANG | LNOMDM;
#	else	/* LNOMDM */
	int		lflag = LNOHANG;
#	endif	/* LNOMDM */

	alarm(20);
#	ifdef	O_NDELAY
	if ( (fd = open(name, O_NDELAY|O_RDWR)) == SYSERROR )
#	else	/* O_NDELAY */
	if ( (fd = open(name, O_RDWR)) == SYSERROR )
#	endif	/* O_NDELAY */
		return fd;
	alarm(0);

	(void)ioctl(fd, TIOCLSET, &lflag);

#	ifdef	O_NDELAY
	if ( fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NDELAY) == SYSERROR )
	{
		(void)close(fd);
		return SYSERROR;
	}
#	endif	/* O_NDELAY */

	return fd;
}
#else	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */



int
opentty(name)
	char *	name;
{
	int	val;

	alarm(20);
	val = open(name, O_RDWR);
	alarm(0);
	return val;
}
#endif	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */
#endif	/* SYSTEM >= 3 */
