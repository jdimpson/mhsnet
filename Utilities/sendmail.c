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
**	Fast, small, mail program for sending network mail.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"exec.h"
#include	"link.h"
#include	"Passwd.h"
#include	"params.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#include	<ctype.h>


/*
**	CONFIGURATION.
*/

#define	EDITOR		"ed"
#define	MORE		"more"
#define	SENDACKARGS	"-au -Amailer -NMail"
#define	SENDARGS	"-u -Amailer -NMail"
#define	SENDER		"sendfile"
#define	XFACELL		70

/*
**	Parameters set from arguments.
*/

bool	Ack;		/* True if mail to be acknowledged */
bool	AllowDotAt = true; /* Last `.' interprested as `@' if no `@' in address */
char *	Bcc;		/* Blind carbon-copy addresses (means separate mail item) */
char *	Cc;		/* Carbon copy addresses */
char *	Date;		/* Composition date */
bool	DeadIn;		/* `dead.letter' incorporated */
char *	FileName;	/* File default for `.m' `.r' */
char *	FromAddr;	/* Composite `From: ' address for mail */
bool	IgnCmds;	/* Ignore `.' commands on `stdin' */
bool	LetterIn;	/* Read header + body from `stdin' */
char *	LogName;	/* Invoker's login name */
bool	MessageID;	/* Add a `Message-ID: ' line */
char *	MIMEVersion;	/* MIME header version */
char *	MIMEContType;	/* MIME header Content-Type */
char *	MIMEContTrEnc;	/* MIME header Content-Transfer-Encoding */
char *	MsgID;		/* `Message-ID: ' unique mail ID */
char *	Name = sccsid;	/* Program name */
bool	NoToEOT;	/* Don't echo to `stdout' */
bool	NoRet;		/* No return requested */
char *	Organisation;	/* `Organization: ' field */
char *	OutBox;		/* Copies of messages */
char *	OutList;	/* Recipient list for OUTBOX directories */
bool	Print;		/* Print contents on startup */
char *	Rcvd;		/* `Received: ' advertising for mail */
char *	RealName;	/* Invoker's real name */
bool	Received;	/* Add a "Received: " line */
char *	ReplyTo;	/* `Real' From address */
bool	ReportID;	/* Report ID of query */
char *	Signature;	/* File containing sender details */
char *	SourceAddr;	/* `From: ' site address for mail */
char *	Subject;	/* `Subject: ' line */
char *	ToAddr;		/* `To: ' addresses for mail */
int	Traceflag;	/* Global tracing control */
char *	XFace;		/* `X-Face: ' field */

/*
**	Arguments.
*/

AFuncv	getBccPrompt _FA_((PassVal, Pointer));	/* Prompt for "Bcc:" */
AFuncv	getCcPrompt _FA_((PassVal, Pointer));	/* Prompt for "Cc:" */
AFuncv	getDlet _FA_((PassVal, Pointer));	/* Incorporate "dead.letter" */
AFuncv	getEnv _FA_((PassVal, Pointer));	/* Environment arguments */
AFuncv	getFlet _FA_((PassVal, Pointer));	/* Incorporate formatted mail from `stdin' */
AFuncv	getNotbool _FA_((PassVal, Pointer));	/* Invert arg */
AFuncv	getSubPrompt _FA_((PassVal, Pointer));	/* Don't prompt for "Subject:" */
AFuncv	getTo _FA_((PassVal, Pointer));		/* Destination addresses */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &Ack, 0),
	Arg_bool(b, 0, getBccPrompt),
	Arg_bool(c, 0, getCcPrompt),
	Arg_bool(d, 0, getDlet),
	Arg_bool(e, &Received, 0),
	Arg_bool(f, 0, getFlet),
	Arg_bool(i, &MessageID, 0),
	Arg_bool(m, &IgnCmds, 0),
	Arg_bool(o, &AllowDotAt, getNotbool),
	Arg_bool(p, &Print, 0),
	Arg_bool(q, &ReportID, 0),
	Arg_bool(r, &NoRet, 0),
	Arg_bool(s, 0, getSubPrompt),
	Arg_bool(t, &NoToEOT, 0),
	Arg_string(F, &FileName, 0, english("file name"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace level"), OPTARG|OPTVAL),
	Arg_noflag(0, getEnv, english("(ADDRESS|BCC|CC|LOGNAME|OUTBOX|NAME|SIGNATURE|SUBJECT)=value"), OPTARG|MANY),
	Arg_noflag(0, getTo, english("to"), OPTARG|MANY),
	Arg_end
};

/*
**	Environment argument types in order of prompt / mail header inclusion.
*/

typedef struct
{
	char *	name;	/* Environment ID */
	char **	p;	/* Value */
	char *	head;	/* Mail header type  description */
	bool	force;	/* Prompt if value not set */
	bool	pref;	/* Environment takes precedence */
	bool	out;	/* Written to message */
	bool	fix;	/* Use `fixaddr()' */
	bool	noenv;	/* Not `getenv()' */
	bool	fixed;	/* Used `fixaddr()' */
	char *	fixedp;	/* Fixed value */
}
	Env;

Env	Envs[] =
{
	{english("ADDRESS"),	&SourceAddr,	english("Site address: "),
		true,	true,	false,	false,	false,	false},
	{english("LOGNAME"),	&LogName,	english("Login name: "),
		true,	true,	false,	false,	false,	false},
	{english("NAME"),	&RealName,	english("Real name: "),
		true,	true,	false,	false,	false,	false},
	{english("RECEIVED"),	&Rcvd,		english("Received: "),
		false,	false,	true,	false,	true,	false},
	{english("DATE"),	&Date,		english("Date: "),
		false,	false,	true,	false,	true,	false},
	{EmptyString,		&FromAddr,	english("From: "),
		false,	false,	true,	true,	true,	false},
#						define	FROM	Envs[5].head
	{EmptyString,		&ReplyTo,	english("Reply-To: "),
		false,	false,	true,	false,	true,	false},
	{english("TO"),		&ToAddr,	english("To: "),
		true,	false,	true,	true,	true,	false},
#						define	TOENV	Envs[7]
#						define	TO	Envs[7].head
	{english("CC"),		&Cc,		english("Cc: "),
		false,	false,	true,	true,	true,	false},
#						define	CCENV	Envs[8]
#						define	CC	Envs[8].head
#						define	FORCCC	Envs[8].force
	{english("BCC"),	&Bcc,		english("Bcc: "),
		false,	false,	true,	true,	true,	false},
#						define	BCCENV	Envs[9]
#						define	BCC	Envs[9].head
#						define	FORCBCC	Envs[9].force
	{english("MESSAGEID"),	&MsgID,		english("Message-ID: "),
		false,	false,	true,	false,	true,	false},
	{english("XFACE"),	&XFace,		english("X-Face: "),
		false,	true,	true,	false,	true,	false},
	{english("ORGANIZATION"), &Organisation, english("Organization: "),
		false,	false,	true,	false,	true,	false},
	{english("SUBJECT"),	&Subject,	english("Subject: "),
		true,	false,	true,	false,	false,	false},
#						define	SUBJECT	Envs[13].head
#						define	FORCSUB	Envs[13].force
	{english("OUTBOX"),	&OutBox,	NULLSTR,
		false,	false,	false,	false,	false,	false},
#						define	OUTBOX	Envs[14].head
	{english("SIGNATURE"),	&Signature,	NULLSTR,
		false,	false,	false,	false,	false,	false},
	{english("MIME-Version"), &MIMEVersion,	english("MIME-Version: "),
		false,	false,	true,	false,	true,	false},
	{english("Content-Type"), &MIMEContType,english("Content-Type: "),
		false,	false,	true,	false,	true,	false},
	{english("Content-Transfer-Encoding"),	&MIMEContTrEnc, english("Content-Transfer-Encoding: "),
		false,	false,	true,	false,	true,	false},
};

#define	NENVS	((sizeof Envs)/sizeof(Env))


/*
**	Memory buffer.
*/

typedef struct
{
	char *	cp;
	char *	buf;
	int	len;
}
	Buf;

Buf	Body;	/* Of mail message */
int	NewLen;	/* Advised buffer size */

/*
**	`uuencode' table.
*/

char	UUTable[64]	=
{
	'`', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_'
};

#define	UUINLINESIZE	45

/*
**	Miscellaneous definitions.
*/

char *	CommaSpace	= ", ";
char	WhiteSpace[]	= " \n\t";
char *	DeadLetter;
bool	Finished;			/* Recursion stopper during windup */
char *	Home;
int	LenHome;
Passwd	Invoker;
char *	ParentName;			/* Execute changes `Name' in child */
bool	SigIn;				/* `.signature' incorporated */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a unique name */
char *	TmpFile;			/* File for elfic commands */

/*
**	Configurable parameters.
*/

char *	Editor		= EDITOR;
char *	Pager		= MORE;
char *	SendAckARGS	= SENDACKARGS;
char *	SendARGS	= SENDARGS;
char *	Sender;

ParTb	Params[] =
{
	{"ACKMAIL",		(char **)&Ack,			PINT},
	{"ADDRESS",		&SourceAddr,			PSTRING},
	{"EDITOR",		&Editor,			PSTRING},
	{"MESSAGEID",		(char **)&MessageID,		PINT},
	{"NORET",		(char **)&NoRet,		PINT},
	{"ORGANIZATION",	&Organisation,			PSTRING},
	{"PAGER",		&Pager,				PSTRING},
	{"RECEIVED",		(char **)&Organisation,		PINT},
	{"SENDACKARGS",		&SendAckARGS,			PSTRING},
	{"SENDARGS",		&SendARGS,			PSTRING},
	{"SENDER",		&Sender,			PSPOOL},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"XFACE",		&XFace,				PSTRING},
};

void	addstr _FA_((char **, char *, char *));
void	allowsigint _FA_((VarArgs *));
char *	buffer _FA_((int, Buf *));
void	bufferstr _FA_((char *, char *));
void	buffertab _FA_((char *, char *));
bool	copybody _FA_((FILE *, bool));
void	copyheader _FA_((FILE *, FILE *, bool));
bool	dead_letter _FA_((bool));
void	dobang _FA_((char *, bool, char **));
void	dobox _FA_((char *, bool, char **));
bool	dodot _FA_((int));
void	doedit _FA_((char *, bool, char **));
void	doline _FA_((char *, bool, char **));
void	dopipe _FA_((char *, bool, char **));
void	dosig _FA_((char *, bool, char **));
int	envcmp _FA_((const void *, const void *));
void	EOT _FA_((void));
bool	fillenvs _FA_((void));
char *	fixaddr _FA_((Env *));
int	getstr _FA_((char **, char *));
char *	homefile _FA_((char *));
void	incorp _FA_((char *, bool, char **));
void	join _FA_((char **, char *, char *));
bool	makefrom _FA_((void));
bool	outbox _FA_((void));
char *	path _FA_((char *));
void	print _FA_((bool));
int	pwrof2 _FA_((int));
void	readdot _FA_((void));
char *	realaddr _FA_((char *));
bool	save _FA_((char *, bool, char **));
char *	scanheader _FA_((char *));
void	sendmsg _FA_((char *, bool));
void	setarg0 _FA_((VarArgs *));
void	setin _FA_((VarArgs *));
char *	StringCMatch _FA_((char *, char *));
void	toggleAck _FA_((bool));
void	uuencode _FA_((char *, char *));

extern SigFunc	catch;

#define	Fprintf	(void)fprintf



/*
**	Argument processing.
*/

int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register char *	cp;
	char		id1[16];
	char		id2[16];

	Finished = true;	/* Want "finish()" to just exit */

	if ( getuid() != geteuid() )
	{
		Fprintf(stderr, english("No permission.\n"));
		exit(EX_NOPERM);	/* SECURITY CHECK FAILED */
	}

	InitParams();	/* Also sets Time and Pid */

	GetParams("netmail", Params, sizeof Params);	/* NB: we don't know `Name' until DoArgs below! */

	if ( !ReadRoute(NOSUMCHECK) )
	{
		Fprintf(stderr, english("No routing tables.\n"));
		exit(EX_OSFILE);
	}

	Home = StripTypes(HomeName);
	LenHome = strlen(Home)+1;

	if ( SourceAddr == NULLSTR )
	{
		if ( (cp = strchr(Home, DOMAIN_SEP)) != NULLSTR && cp[1] != '\0' && StringMatch(HomeName, "3=") != NULLSTR )
			SourceAddr = cp+1;	/* Default `site' address */
		else
			SourceAddr = Home;
	}

	GetInvoker(&Invoker);

	if ( (cp = Invoker.P_rname) != NULLSTR && *cp != '\0' )
		RealName = newstr(cp);
	LogName = newstr(Invoker.P_name);

	Date = newstr(rfc822time(&Time));
	Date[31] = '\0';	/* Clobber '\n' */

	DeadLetter = homefile("dead.letter");

	DoArgs(argc, argv, Usage);

	if ( !Print && !DeadIn && !LetterIn && ToAddr != NULLSTR && !NoToEOT )
		Fprintf(stdout, "%s%s\n", TO, fixaddr(&TOENV));

	if ( Print && makefrom() )
	{
		(void)fillenvs();
		FreeStr(&FromAddr);
	}
	else
		(void)fillenvs();

	(void)makefrom();

	if ( Received && Rcvd == NULLSTR )
		Rcvd = concat("by ", Home, NULLSTR);

	if ( MessageID && MsgID == NULLSTR )
	{
		(void)EncodeNum(id1, Time, -1);
		(void)EncodeNum(id2, Pid, -1);
		MsgID = concat("<", id1, id2, "@", Home, ">", NULLSTR);
	}

	if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGINT, catch);

	if ( (SigFunc *)signal(SIGTERM, SIG_IGN) != SIG_IGN )
		(void)signal(SIGTERM, catch);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	TmpFile = concat(TMPDIR, Template, NULLSTR);

	Finished = false;
	ParentName = Name;

	if ( LetterIn )
		EOT();	/** No return **/

	if ( !NoToEOT )
	{
		if ( DeadIn )
		{
			if ( !Print )
				copyheader(stdout, stdout, false);
			Fprintf(stdout, english("...\n\n(continue)\n"));
		}
		else
			putc('\n', stdout);
	}

	readdot();

	return 0;
}



/*
**	Add data to specific field with separator `f'.
*/

void
addstr(p, s, f)
	char **		p;
	char *		s;
	char *		f;
{
	register int	c;
	register bool	non_empty;
	Buf		buf;

	Trace4(1, "addstr(%s, %s, %s)", *p, s, (f==NULLSTR)?EmptyString:f);

	buf.buf = NULLSTR;

	if ( s != NULLSTR )
		Fprintf(stdout, "%s", s);
	if ( *p != NULLSTR )
		Fprintf(stdout, "%s ", *p);

	non_empty = false;

	while ( (c = getc(stdin)) != EOF )
	{
		if ( c == '\n' )
			break;

		(void)buffer(c, &buf);

		non_empty = true;
	}

	if ( non_empty )
	{
		(void)buffer('\0', &buf);
		join(p, buf.buf, f);
		free(buf.buf);
	}
}



/*
**	Turn on SIGINT for child.
*/

void
allowsigint(vap)
	VarArgs *	vap;
{
	(void)signal(SIGINT, SIG_DFL);
}



/*
**	Store char in expandable memory buffer.
*/

char *
buffer(c, bufp)
	int		c;
	register Buf *	bufp;
{
	register char *	cp;
	register int	len;

	Trace3(5, "buffer(\\%o, %s)", c, (bufp==&Body)?"Body":"temp");

	if ( !isprint(c) && (c != '\0' || bufp == &Body) )
	{
		char	X[ULONG_SIZE];

		(void)buffer('\\', bufp);	/* Also allocates space */

		switch ( c )
		{
		case '\t':
		case '\n':	bufp->cp[-1] = c;
				return bufp->buf;

		case '\b':	return buffer('b', bufp);
		case '\r':	return buffer('r', bufp);
		case '\f':	return buffer('f', bufp);
		case '\0':	return buffer('0', bufp);
		}

		(void)sprintf(X, "%03o", (Uchar)c);
		(void)buffer(X[0], bufp);
		(void)buffer(X[1], bufp);
		return buffer(X[2], bufp);
	}

	if ( (cp = bufp->buf) == NULLSTR )
	{
		bufp->len = pwrof2(NewLen); NewLen = 0;
		bufp->buf = bufp->cp = cp = Malloc(bufp->len);
		Trace2(2, "buffer: malloc first %d bytes", bufp->len);
	}
	else
	if ( (bufp->cp - cp) >= (len = bufp->len) )
	{
		register int	newlen;

		if ( (newlen = NewLen) > 0 )
			newlen = pwrof2(newlen + len);
		else
		if ( (newlen = len) > 8192 )
			newlen += 8192;
		else
			newlen *= 2;
		Trace4(3, "buffer: need %d bytes (NewLen = %d, len = %d)", newlen, NewLen, len);
		NewLen = 0;
		bufp->buf = Malloc(newlen);
		(void)strncpy(bufp->buf, cp, len);
		free(cp);
		bufp->cp = &bufp->buf[len];
		bufp->len = newlen;
		cp = bufp->buf;
		Trace4(2, "buffer: malloc %d bytes {%.*s}", newlen, len, cp);
	}

	if ( (*bufp->cp++ = c) != '\0' )
	{
		Trace2(3, "buffer('%c')", c);
	}
	else
		Trace1(3, "buffer('\\0')");

	return cp;
}



/*
**	Copy string into message buffer.
*/

void
bufferstr(file, data)
	char *		file;
	char *		data;
{
	register int	c;
	register char *	cp;
	register Buf *	bufp = &Body;

	Trace3(1, "bufferstr(%s, %#lx)", file, (PUlong)data);

	for ( cp = data ; (c = *cp++) != '\0' ; )
		(void)buffer(c, bufp);
}



/*
**	Copy string into message buffer with leading tab.
*/

void
buffertab(file, data)
	char *		file;
	char *		data;
{
	register int	c;
	register char *	cp;
	register bool	tab = true;
	register Buf *	bufp = &Body;

	Trace3(1, "buffertab(%s, %#lx)", file, (PUlong)data);

	for ( cp = data ; (c = *cp++) != '\0' ; )
	{
		if ( tab && c != '\n' )
		{
			(void)buffer('\t', bufp);
			tab = false;
		}

		(void)buffer(c, bufp);

		if ( c == '\n' )
			tab = true;
	}
}



/*
**	Catch signal, save message in "dead.letter".
*/

void
catch(sig)
	int		sig;
{
	static bool	second_time;

	(void)signal(sig, SIG_IGN);

	if ( Finished )
		second_time = true;
	else
	{
		second_time = false;
		Finished = true;
	}

	Fprintf(stdout, english("INTR\n"));

	if ( dead_letter(second_time) )
		exit(EX_TEMPFAIL);

	(void)signal(sig, catch);
	Fprintf(stdout, english("One more to QUIT\n(continue)\n"));
}



/*
**	Explain commands.
*/

void
commands()
{
	Fprintf(stdout, english("\
Commands:-\n\
.! prog [arg ...]	execute unix program\n\
.| prog [arg ...]	pipe body of message through `prog'\n\
.< prog [arg ...]	append output of `prog'\n\
.?/.h			this message\n\
.a/.A			toggle / set message acknowledgement status\n\
.b/.B [list]		add / set blind carbon copy recipients\n\
.c/.C [list]		add / set carbon copy recipients\n\
.d/.D			append / substitute `dead.letter'\n\
.e/.E [editor]		edit whole / body of message\n"));

	/* Split because some cc's can't hack it. */

	Fprintf(stdout, english("\
.f/.F			add / set outbox recipient list\n\
.g/.G [file]		append / substitute signature file\n\
.m/.M [file]		append / substitute `file' >> one tab\n\
.o/.O [list]		add / set outbox directory list\n\
.p/.P			print whole / body of message\n\
.r/.R [file]		append / substitute `file'\n\
.s/.S			add / set `Subject' line\n\
.t/.T [list]		add / set `To' line\n\
.u/.U [file]		append / substitute `file' uuencoded\n\
.w/.W file		append to / overwrite `file'\n\
"));
}



/*
**	Write out message `body'.
*/

bool
copybody(fd, clean)
	FILE *		fd;
	bool		clean;	/* Ensure exactly 1 trailing newline */
{
	register char *	bp;
	register char *	cp;

	Trace3(1, "copybody(%d, %s)", fileno(fd), clean?"clean":EmptyString);

	(void)fflush(fd);

	if ( (bp = Body.buf) == NULLSTR )
		return false;

	(void)buffer('0', &Body);
	*--(Body.cp) = '\0';	/* Ensure terminating '\0' for body */

	cp = Body.cp;
	bp = Body.buf;		/* `buffer' above may have changed Body! */

	if ( clean )
	{
		register char *	mp;
		register bool	val = false;
		register int	mlen;
		register int	rlen;

		static char	match[]		= "\nFrom ";
		static char	Replace[]	= "\n>From ";
		static char	replace[]	= "\n>from ";

		mlen = strlen(match);
		rlen = strlen(replace);

		while ( cp > bp && cp[-1] == '\n' )
			--cp;

		if ( strnccmp(bp, match+1, mlen-1) == STREQUAL )
		{
			mp = bp-1;
			goto top;
		}

		while ( (mp = StringCMatch(bp, match)) != NULLSTR )
		{
			(void)write(fileno(fd), bp, mp - bp);
top:			if ( strcmp(mp, match) == STREQUAL )
				(void)write(fileno(fd), Replace, rlen);
			else
				(void)write(fileno(fd), replace, rlen);
			bp = mp + mlen;
			val = true;
		}

		if ( val && cp == bp )
			goto out;
	}

	if ( cp == bp )
		return false;

	(void)write(fileno(fd), bp, cp - bp);

	if ( clean )
out:		putc('\n', fd);

	Trace3(2, "copybody [%d] => %.16s", (cp-Body.buf)+1, Body.buf);
	return true;
}



/*
**	Output header, "fd1" for mail data, "fd2" for `invisible' data.
*/

void
copyheader(fd1, fd2, nofix)
	FILE *		fd1;
	FILE *		fd2;
	bool		nofix;	/* Use original string */
{
	register Env *	envp;
	register char *	cp;
	register FILE *	fd;

	Trace4(1, "copyheader(%d, %d, %sfix)", fileno(fd1), fd2==(FILE *)0?-1:fileno(fd2), nofix?"no":EmptyString);

	for ( envp = Envs ; envp < &Envs[NENVS] ; envp++ )
	{
		if ( !envp->out || (cp = *(envp->p)) == NULLSTR )
			continue;

		if ( envp->p != &Bcc )
			fd = fd1;
		else
		if ( (fd = fd2) == (FILE *)0 )
			continue;

		Trace3(2, "line: %s%s", envp->head, cp);

		if ( !nofix && envp->fix )
		{
			if ( !envp->fixed )
				cp = fixaddr(envp);
			else
				cp = envp->fixedp;
		}

		Fprintf(fd, "%s%s\n", envp->head, cp);
	}

	putc('\n', fd1);
}



/*
**	Count newlines in message.
*/

int
countlines(header)
	bool		header;
{
	register char *	bp;
	register char *	cp;
	register Env *	envp;
	register int	lines = 0;

	if ( (bp = Body.buf) == NULLSTR )
		return lines;

	cp = Body.cp;

	while ( bp < cp )
		if ( *bp++ == '\n' )
			lines++;

	if ( !header )
		return lines;

	lines++;	/* Trailing '\n' */

	for ( envp = Envs ; envp < &Envs[NENVS] ; envp++ )
	{
		if ( !envp->out || (cp = *(envp->p)) == NULLSTR )
			continue;

		do
			lines++;
		while
			( (cp = strchr(++cp, '\n')) != NULLSTR );
	}

	return lines;
}



/*
**	Create the eponymous file.
*/

bool
dead_letter(retry)
	bool	retry;
{
	if ( DeadLetter == NULLSTR )
		return true;

	if ( save(DeadLetter, true, (retry?(char **)0:(char **)1)) )	/* APPEND, force */
	{
		Fprintf(stdout, english("Mail saved in %s\n"), DeadLetter);
		return true;
	}

	return retry;
}



/*
**	Invoke Unix program.
*/

void
dobang(prog, append, vp)
	char *	prog;
	bool	append;
	char **	vp;
{
	char *	errs;
	VarArgs *vap;
	ExBuf	exbuf;

	Trace4(1, "dobang(%s, %d, %#lx)", prog, (int)append, (PUlong)vp);

	(void)signal(SIGINT, SIG_IGN);

	vap = &exbuf.ex_cmd;
	FIRSTARG(vap) = SHELL;
	NEXTARG(vap) = "-c";
	NEXTARG(vap) = prog;

	if ( (errs = Execute(&exbuf, setin, ex_exec, SYSERROR)) != NULLSTR )
		free(errs);

	(void)signal(SIGINT, catch);
}



/*
**	Necessary to pre-set `OutList' from current `ToAddr'.
*/
/*ARGSUSED*/
void
dobox(name, append, vp)
	char *		name;
	bool		append;
	char **		vp;
{
	register char *	cp;
	static char	prompt[]	= "Outbox recipient list: ";

	if ( OutList == NULLSTR )
	{
		if ( ToAddr != NULLSTR )
		{
			OutList = newstr(fixaddr(&TOENV));
			if ( (cp = strpbrk(OutList, "@ ")) != NULLSTR )
				*cp = '\0';
		}
	}

	if ( name != NULLSTR )
	{
		doline(name, append, &OutList);
		return;
	}

	if ( append )
		addstr(&OutList, prompt, NULLSTR);
	else
		(void)getstr(&OutList, prompt);
}



/*
**	Process compose command.
*/

bool
dodot(c1)
	int	c1;
{
	int	c2;
	char **	vp;
	char *	name;
#	ifdef	ANSI_C
	void	(*funcp)(char *, bool, char **);
#	else	/* ANSI_C */
	vFuncp	funcp;
#	endif	/* ANSI_C */
	bool	append;

	vp = (char **)0;

	if ( (c2 = getc(stdin)) == '\n' )
	{
		switch ( c1 )
		{
		case 'a':	toggleAck(false);		return true;
		case 'A':	toggleAck(true);		return true;
		case 'b':	addstr(&Bcc, BCC, CommaSpace);
				BCCENV.fixed = false;		break;
		case 'B':	(void)getstr(&Bcc, BCC);
				BCCENV.fixed = false;		break;
		case 'c':	addstr(&Cc, CC, CommaSpace);
				CCENV.fixed = false;		break;
		case 'C':	(void)getstr(&Cc, CC);
				CCENV.fixed = false;		break;
		case 'd':	incorp(DeadLetter, true, vp);	break;
		case 'D':	incorp(DeadLetter, false, vp);	break;
		case 'e':	doedit(NULLSTR, true, vp);	break;
		case 'E':	doedit(NULLSTR, false, vp);	break;
		case 'f':	dobox(NULLSTR, true, vp);	break;
		case 'F':	dobox(NULLSTR, false, vp);	break;
		case 'g':	dosig(NULLSTR, true, vp);	break;
		case 'G':	dosig(NULLSTR, false, vp);	break;
		case '?':
		case 'h':	commands();			break;
		case 'm':	incorp(FileName, true, (char **)buffertab);	break;
		case 'M':	incorp(FileName, false, (char **)buffertab);	break;
		case 'o':	addstr(&OutBox, OUTBOX, NULLSTR);break;
		case 'O':	(void)getstr(&OutBox, OUTBOX);	break;
		case 'p':	print(true);			break;
		case 'P':	print(false);			break;
		case 'r':	incorp(FileName, true, vp);	break;
		case 'R':	incorp(FileName, false, vp);	break;
		case 's':	addstr(&Subject, SUBJECT, NULLSTR);break;
		case 'S':	(void)getstr(&Subject, SUBJECT);break;
		case 't':	addstr(&ToAddr, TO, CommaSpace);
				TOENV.fixed = false;		break;
		case 'T':	(void)getstr(&ToAddr, TO);
				TOENV.fixed = false;		break;
		case 'u':	incorp(FileName, true, (char **)uuencode);	break;
		case 'U':	incorp(FileName, false, (char **)uuencode);	break;
		case 'w':	(void)save(NULLSTR, true, vp);	break;
		case 'W':	(void)save(NULLSTR, false, vp);	break;

		default:
unrec:			(void)buffer('.', &Body);
			(void)buffer(c1, &Body);

			if ( c2 != EOF )
				(void)buffer(c2, &Body);

			return (c2 == '\n') ? true : false;
		}
out:
		if ( !NoToEOT ) Fprintf(stdout, english("(continue)\n"));
		return true;
	}

	append = false;

	switch ( c1 )
	{
	default:					goto unrec;

	case '!':	funcp = dobang;			break;
	case '<':	append = true;
	case '|':	funcp = dopipe;			break;
	case 'b':	append = true;
	case 'B':	funcp = doline;
			vp = &Bcc;
			BCCENV.fixed = false;		break;
	case 'c':	append = true;
	case 'C':	funcp = doline;
			vp = &Cc;
			CCENV.fixed = false;		break;
	case 'e':	append = true;
	case 'E':	funcp = doedit;			break;
	case 'f':	append = true;
	case 'F':	funcp = dobox;			break;
	case 'g':	append = true;
	case 'G':	funcp = dosig;			break;
	case 'm':	append = true;
	case 'M':	funcp = incorp;	vp = (char **)buffertab;	break;
	case 'o':	append = true;
	case 'O':	funcp = doline;	vp = &OutBox;	break;
	case 'r':	append = true;
	case 'R':	funcp = incorp;			break;
	case 's':	append = true;
	case 'S':	funcp = doline;	vp = &Subject;	break;
	case 't':	append = true;
	case 'T':	funcp = doline;
			vp = &ToAddr;
			TOENV.fixed = false;		break;
	case 'u':	append = true;
	case 'U':	funcp = incorp;	vp = (char **)uuencode;	break;
	case 'w':	append = true;
#	ifdef	ANSI_C
	case 'W':	funcp = (void(*)(char*,bool,char**))save;	break;
#	else	/* ANSI_C */
	case 'W':	funcp = (vFuncp)save;		break;
#	endif	/* ANSI_C */
	}

	if ( c2 != ' ' )
		(void)ungetc(c2, stdin);

	name = NULLSTR;

	if ( getstr(&name, NULLSTR) == EOF )
		EOT();

	(*funcp)(name, append, vp);

	if ( name != NULLSTR )
		free(name);

	goto out;
}



/*
**	Invoke editor to edit message.
*/

void
doedit(name, editall, vp)
	char *		name;
	bool		editall;
	char **		vp;
{
	int	fd;
	FILE *	Fd;
	char *	prog;
	bool	append;
	ExBuf	exbuf;

	static char	ederr[]	= english("(editor error - data unchanged)\n");

	(void)signal(SIGINT, SIG_IGN);

	(void)UniqueName(TmpFile, CHOOSE_QUALITY, OPTNAME, (Ulong)Body.len, Time);
	fd = create(TmpFile);
	Fd = fdopen(fd, "w");

	if ( editall )
	{
		copyheader(Fd, Fd, true);
		append = false;		/* Replace */
	}
	else
		append = true;		/* Append */

	(void)copybody(Fd, false);

	if ( fclose(Fd) == EOF )
	{
		(void)SysWarn(CouldNot, WriteStr, TmpFile);
		Fprintf(stdout, "%s", ederr);
	}
	else
	{
		VarArgs *vap;

		if ( name != NULLSTR )
			prog = name;
		else
		if ( (prog = getenv("EDITOR")) == NULLSTR || prog[0] == '\0' )
			prog = Editor;

		vap = &exbuf.ex_cmd;
		FIRSTARG(vap) = SHELL;
		NEXTARG(vap) = "-c";
		NEXTARG(vap) = newprintf("%s %s", prog, TmpFile);

		if ( (prog = Execute(&exbuf, setin, ex_exec, SYSERROR)) != NULLSTR )
		{
			Fprintf(stdout, "%s", ederr);
			Trace2(1, "stderr => \\\n\"%s\"\n", prog);
			free(prog);
		}
		else
		{
			if ( append )			/* Ensure replace by incorp() */
				Body.cp = Body.buf;	/* Premature before */
			incorp(TmpFile, append, (char **)0);
		}

		free(LASTARG(&exbuf.ex_cmd));
	}

	(void)unlink(TmpFile);

	(void)signal(SIGINT, catch);
}



/*
**	Modify string value.
*/

void
doline(name, append, vp)
	char *	name;
	bool	append;
	char **	vp;
{
	Trace4(1, "doline(%s, %s, %s)", name, append?"append":"replace", *vp);

	if ( append )
	{
		join(vp, name, NULLSTR);
		return;
	}

	FreeStr(vp);
	*vp = newstr(name);
}



/*
**	Invoke program to format body, or incorporate output.
*/

void
dopipe(prog, append, vp)
	char *	prog;
	bool	append;
	char **	vp;
{
	FILE *	fd;
	int	ofd;
	char *	errs;
	VarArgs *vap;
	ExBuf	exbuf;

	(void)signal(SIGINT, SIG_IGN);

	(void)UniqueName(TmpFile, CHOOSE_QUALITY, OPTNAME, (Ulong)Body.len, Time);
	ofd = creater(TmpFile);

	vap = &exbuf.ex_cmd;
	FIRSTARG(vap) = SHELL;
	NEXTARG(vap) = "-c";
	NEXTARG(vap) = prog;

	fd = (FILE *)Execute(&exbuf, allowsigint, ex_pipo, ofd);

	(void)close(ofd);

	if ( !append )
		(void)copybody(fd, false);

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Fprintf(stdout, english("(pipe error - data unchanged)\n"));
		free(errs);
	}
	else
		incorp(TmpFile, append, (char **)bufferstr);

	(void)unlink(TmpFile);

	(void)signal(SIGINT, catch);
}



/*
**	Incorporate signature file.
*/

void
dosig(file, append, vp)
	char *		file;
	bool		append;
	char **		vp;
{
#	if	0
	register char *	cp;
	register int	c;
	static char	sep[]	= "\n--\n";
#	endif	/* 0 */

	if ( file != NULLSTR )
		Signature = newstr(file);

	if ( Signature == NULLSTR || Signature[0] == '\0' )
		Signature = homefile(".signature");

#	if	0
	for ( cp = sep ; (c = *cp++) != '\0' ; )
		(void)buffer(c, &Body);
#	else	/* 0 */
		(void)buffer('\n', &Body);
#	endif	/* 0 */

	incorp(Signature, append, (char **)bufferstr);

	SigIn = true;
}



/*
**	Compare routine for environment fields.
*/

int
envcmp(cp, dp)
	const void *	cp;
	const void *	dp;
{
	return strcmp((char *)cp, ((Env *)dp)->name);
}



/*
**	Wind up and transmit message.
*/

void
EOT()
{
	char *	to;
	bool	assure;

	if ( !NoToEOT )
		Fprintf(stdout, "EOT\n");

	if ( Signature != NULLSTR && !SigIn )
		dosig(NULLSTR, true, (char **)0);

	if ( ToAddr == NULLSTR || strlen(ToAddr) == 0 )
		Error(english("null address"));

	if ( !outbox() )
	{
		(void)dead_letter(true);
		assure = true;
	}
	else
		assure = false;

	if ( Cc != NULLSTR )
		to = realaddr(concat(fixaddr(&TOENV), CommaSpace, fixaddr(&CCENV), NULLSTR));
	else
		to = realaddr(fixaddr(&TOENV));

	sendmsg(to, false);

	if ( Bcc != NULLSTR )
		sendmsg(realaddr(fixaddr(&BCCENV)), true);

	if ( assure )
		Fprintf(ErrorFd, "Mail sent.\n");

	exit(EX_OK);
}



/*
**	Try filling missing fields from environment variables.
*/

bool
fillenvs()
{
	register Env *	envp;
	register char *	cp;
	register bool	result = false;

	Trace1(1, "fillenvs()");

	for ( envp = Envs ; envp < &Envs[NENVS] ; envp++ )
	{
		if ( envp->noenv || envp->name[0] == '\0' || (cp = getenv(envp->name)) == NULLSTR || cp[0] == '\0' )
		{
			Trace2(3, "%s: <null>", envp->name);

			if ( *(envp->p) != NULLSTR || !envp->force || DeadIn || NoToEOT )
				goto print;

			do
			{
				result = true;

				if ( getstr(envp->p, envp->head) == EOF )
				{
					putc('\n', stdout);
					exit(EX_NOINPUT);
				}
			}
			while
				( *(envp->p) == NULLSTR && envp->p != &Subject );

			envp->fixed = false;
			continue;
		}

		Trace3(3, "%s: %s", envp->name, cp);

		if ( *(envp->p) == NULLSTR || envp->pref )
		{
			*(envp->p) = newstr(cp);
			envp->fixed = false;
		}

print:		if ( Print && envp->out && (cp = *(envp->p)) != NULLSTR && *cp != '\0' )
		{
			if ( envp->fix && !envp->fixed )
				cp = fixaddr(envp);

			Fprintf(stdout, "%s%s\n", envp->head, cp);
		}
	}

	return result;
}



void
finish(err)
	int		err;
{
	if ( Finished || ExInChild )
		exit(err);

	Finished = true;
	catch(SIGINT);
}



/*
**	Ensure '@' exists, and expand addresses.
*/

char *
fixaddr(envp)
	Env *		envp;
{
	register char *	cp;
	register char *	sp;
	register char *	np;
	register char *	xp;
	register char *	ep;
	register int	len;
	char *		nap;
	char *		tail;
	char *		comma;
	char *		copysp;
	static char	complex[]	= "<(";

	if ( (sp = *(envp->p)) == NULLSTR )
		return sp;

	if ( envp->fixed )
	{
		cp = envp->fixedp;
		goto fixed;
	}

	Trace2(2, "fixaddr(%s)", sp);

	if ( strpbrk(sp, complex) != NULLSTR )
		comma = ",";
	else
		comma = CommaSpace;

	len = strlen(sp) + LenHome;

	cp = sp;

	while ( (np = strpbrk(cp, comma)) != NULLSTR )
	{
		len += LenHome;
		cp = np + strspn(np, comma);
	}
again:
	Trace2(3, "fixaddr len ==> %d", len);

	copysp = sp = newstr(sp);

	nap = xp = Malloc(len+100);
	ep = xp + len;

	for ( ;; )
	{
		sp += strspn(sp, CommaSpace);

		if ( (np = strpbrk(sp, "\"(,")) != NULLSTR )
		{
			switch ( *np )
			{
			case '"':	cp = strchr(np+1, '"');	break;
			case '(':	cp = strchr(np+1, ')');	break;
			case ',':	cp = NULLSTR;		break;
			}
			if ( cp != NULLSTR )
				np = strchr(cp+1, ',');
			if ( np != NULLSTR )
				*np++ = '\0';
		}

		Trace2(4, "fixaddr %s", sp);

		if ( (cp = strchr(sp, '<')) != NULLSTR )
		{
			cp++;
			xp = strncpyend(xp, sp, cp-sp);
			sp = cp;
			if ( (cp = strchr(sp, '>')) != NULLSTR )
			{
				tail = newstr(cp);
				*cp = '\0';
			}
			else
				tail = newstr(">");
		}
		else
		if ( (cp = strpbrk(sp, "\"(")) != NULLSTR )
		{
			char	c[2];

			switch ( *cp )
			{
			case '(':	c[0] = ')';	break;
			case '"':	c[0] = '"';	break;
			}
			c[1] = '\0';

			if ( (tail = strchr(cp+1, c[0])) != NULLSTR )
			{
				if ( (cp-sp) > (int)strlen(tail) )
				{
					tail = concat(" ", cp, NULLSTR);
					*cp = '\0';
					while ( cp[-1] == ' ' )
						*--cp = '\0';
				}
				else
				{
					xp = strncpyend(xp, sp, ++tail-sp);
					*xp++ = ' ';
					while ( *tail == ' ' )
						tail++;
					sp = tail;
					tail = NULLSTR;
				}
			}
			else
			{
				tail = concat(" ", cp, c, NULLSTR);
				*cp = '\0';
				while ( cp[-1] == ' ' )
					*--cp = '\0';
			}
		}
		else
			tail = NULLSTR;

		if
		(
			(cp = strrchr(sp, '@')) != NULLSTR
			||
			(AllowDotAt && (cp = strchr(sp, '.')) != NULLSTR)
		)
		{
			Addr *	ap;
			char *	up;

			*cp++ = '\0';
			xp = strcpyend(xp, sp);
			*xp++ = '@';	/* Turn first '.' to an '@' */
			if ( (ap = StringToAddr(cp, NO_STA_MAP)) != (Addr *)0 )
			{
				up = UnTypAddress(ap);
				if ( *up == ATYP_DESTINATION )
					up++;
			}
			else
				up = Home;
			xp = strcpyend(xp, up);
			FreeAddr(&ap);
		}
		else
		{
			xp = strcpyend(xp, sp);
			*xp++ = '@';
			xp = strcpyend(xp, Home);
		}

		if ( tail != NULLSTR )
		{
			xp = strcpyend(xp, tail);
			free(tail);
		}

		if ( (sp = np) == NULLSTR )
			break;

		sp += strspn(sp, WhiteSpace);
		if ( *sp == '\0' )
			break;

		*xp++ = ',';			/* RFC822 separator */
		*xp++ = ' ';

		if ( xp > ep )
		{
			sp = *(envp->p);
			len += 100;
			free(nap);
			free(copysp);
			goto again;
		}
	}

	cp = newstr(nap);
	free(nap);
	free(copysp);
	envp->fixedp = cp;
	envp->fixed = true;
fixed:
	Trace3(3, "fixaddr(%s) ==> %s", *(envp->p), cp);

	return cp;
}



/*
**	Prompt for "Bcc:".
*/

/*ARGSUSED*/
AFuncv
getBccPrompt(val, arg)
	PassVal		val;
	Pointer		arg;
{
	FORCBCC = true;
	return ACCEPTARG;
}



/*
**	Prompt for "Cc:".
*/

/*ARGSUSED*/
AFuncv
getCcPrompt(val, arg)
	PassVal		val;
	Pointer		arg;
{
	FORCCC = true;
	return ACCEPTARG;
}



/*
**	Incorporate dead.letter.
*/

/*ARGSUSED*/
AFuncv
getDlet(val, arg)
	PassVal		val;
	Pointer		arg;
{
	DeadIn = true;
	incorp(DeadLetter, false, (char **)0);
	return ACCEPTARG;
}



/*
**	Get optional definitions for mail.
*/

/*ARGSUSED*/
AFuncv
getEnv(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register Env *	envp;
	register char *	cp;
	size_t		n = NENVS;

	if ( (cp = strchr(val.p, '=')) == NULLSTR )
		return REJECTARG;

	*cp++ = '\0';

	if ( (envp = (Env *)lfind(val.p, (char *)Envs, &n, sizeof(Env), envcmp)) == (Env *)0 )
	{
		*--cp = '=';	/* Might be typed address */
		return REJECTARG;
	}

	if ( envp->p == &XFace )
	{
		register char *	xp;
		register char *	np;
		register int	i;

		/** Strip newline+tab **/

		for ( xp = cp ; (np = strchr(xp, '\n')) != NULLSTR ; xp = np )
		{
			for ( i = 1 ; np[i] == '\t' || np[i] == ' ' ; i++ );
			(void)strcpy(np, &np[i]);
			np += i;
		}

		/** Add newline+tab at XFACELL intervals **/

		i = strlen(cp);
		*(envp->p) = np = Malloc(i+1+(i/XFACELL)*2);
		for ( ;; )
		{
			np = strncpyend(np, cp, (i>XFACELL)?XFACELL:i);
			if ( (i-= XFACELL) <= 0 )
			{
				*np = '\0';
				break;
			}
			*np++ = '\n';
			*np++ = '\t';
			cp += XFACELL;
		}
	}
	else
		*(envp->p) = newstr(cp);

	envp->pref = false;
	envp->fixed = false;

	return ACCEPTARG;
}



/*
**	Incorporate letter with header from `stdin'.
*/

/*ARGSUSED*/
AFuncv
getFlet(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	register int	c;
	register bool	newline = false;

	LetterIn = NoToEOT = true;
	FORCSUB = false;

	while ( (c = getc(stdin)) != EOF )
	{
		switch ( c )
		{
		case '\n':	newline = true;
				break;
		default:	newline = false;
		}

		(void)buffer(c, &Body);
	}

	if ( !newline )
		(void)buffer('\n', &Body);

	(void)buffer('0', &Body);
	*--(Body.cp) = '\0';	/* Ensure terminating '\0' for header */

	if ( (cp = scanheader(Body.buf)) > Body.buf )
	{
		if ( (c = Body.cp - cp) > 0 )
			(void)strncpy(Body.buf, cp, c);
		Body.cp = Body.buf + c;
		*(Body.cp) = '\0';
		Trace3(2, "getFlet body [%d] => %.16s", c, Body.buf)
	}

	return ACCEPTARG;
}



/*
**	Invert boolean.
*/

/*ARGSUSED*/
AFuncv
getNotbool(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(bool *)arg = false;
	return ACCEPTARG;
}



/*
**	Get a line for specific field.
*/

int
getstr(p, s)
	char **		p;
	char *		s;
{
	register int	c;
	register bool	non_empty;
	Buf		buf;

	Trace3(1, "getstr(%s, %s)", (*p==NULLSTR)?NullStr:*p, (s==NULLSTR)?NullStr:s);

	FreeStr(p);

	buf.buf = NULLSTR;

	if ( s != NULLSTR )
		Fprintf(stdout, "%s", s);

	non_empty = false;

	while ( (c = getc(stdin)) != EOF )
	{
		if ( c == '\n' )
			break;

		(void)buffer(c, &buf);

		non_empty = true;
	}

	if ( non_empty )
	{
		*p = newstr(buffer('\0', &buf));
		free(buf.buf);
	}

	return c;
}



/*
**	Don't prompt for "Subject:".
*/

/*ARGSUSED*/
AFuncv
getSubPrompt(val, arg)
	PassVal		val;
	Pointer		arg;
{
	FORCSUB = false;
	return ACCEPTARG;
}



/*
**	Get optional destinations for mail.
*/

/*ARGSUSED*/
AFuncv
getTo(val, arg)
	PassVal		val;
	Pointer		arg;
{
	int		l = strlen(val.p);

	if ( l > 0 && val.p[l-1] == ',' )
		val.p[--l] = '\0';

	if ( l > 0 )
	{
		join(&ToAddr, val.p, CommaSpace);
		TOENV.fixed = false;
	}

	return ACCEPTARG;
}



/*
**	Setup default filename in HOME.
*/

char *
homefile(fn)
	char *	fn;
{
	int	len;

	if ( (len = strlen(Invoker.P_dir)) > 0 )
	{
		if ( Invoker.P_dir[len-1] == '/'  )
			return concat(Invoker.P_dir, fn, NULLSTR);
		else
			return concat(Invoker.P_dir, Slash, fn, NULLSTR);
	}

	return fn;
}



/*
**	Incorporate another file.
*/

void
incorp(file, append, vp)
	char *		file;
	bool		append;
	register char **vp;	/* Zero&&!append means scanheader, non-zero means funcp */
{
	register char *	cp;
	char *		data;

	Trace4(1, "incorp(%s, %d, %#lx)", file, (int)append, (PUlong)vp);

	if ( (data = cp = ReadFile(file)) == NULLSTR )
	{
		if ( errno )
			(void)SysWarn(CouldNot, ReadStr, file);
		else
		if ( file == NULLSTR )
			Fprintf(stdout, english("(file name?)\n"));

		return;	/* Probably empty file */
	}

	/** ReadFile removes last '\n' **/

	if ( !append )
	{
		Body.cp = Body.buf;

		if ( vp == (char **)0 )
			cp = scanheader(cp);
	}

	NewLen = RdFileSize+1;	/* Advise size */

	if ( vp != (char **)0 )
		(*(vFuncp)vp)(file, cp);
	else
	{
		register char *	ep;
		register Buf *	bufp = &Body;

		for ( ep = data+RdFileSize ; cp < ep ; )
			(void)buffer(*cp++, bufp);
	}

	if ( Body.cp != Body.buf )
		(void)buffer('\n', &Body);

	free(data);
}



/*
**	Concatenate two strings with a separator (default: space).
*/

void
join(a1, a2, a3)
	register char **a1;
	char *		a2;
	char *		a3;
{
	register char *	cp;

	if ( (cp = *a1) == NULLSTR )
	{
		Trace2(1, "join(, %s, )", a2);
		*a1 = newstr(a2);
		return;
	}

	if ( a3 == NULLSTR )
		a3 = " ";

	Trace4(1, "join(%s, %s, %s)", cp, a2, a3);

	*a1 = concat(cp, a3, a2, NULLSTR);

	free(cp);
}



/*
**	Make `FromAddr' if null.
*/

bool
makefrom()
{
	if ( FromAddr != NULLSTR )
		return false;

	if ( RealName != NULLSTR && RealName[0] != '\0' )
		FromAddr = concat(LogName, "@", SourceAddr, " (", RealName, ")", NULLSTR);
	else
		FromAddr = concat(LogName, "@", SourceAddr, NULLSTR);

	return true;
}



/*
**	Append message to OUTBOX(es).
*/

bool
outbox()
{
	register char *	fn;
	register char *	cp;
	register FILE *	fd;
	register char *	ep;
	char *		name;
	char *		nn;
	char *		sn;
	bool		first;
	bool		retval;
	struct stat	statb;
	static char	seps[] = " ,";

	if ( (cp = OutList) == NULLSTR )
	{
		name = sn = realaddr(fixaddr(&TOENV));
		if ( (cp = strpbrk(name, "@ ")) != NULLSTR )
			*cp = '\0';
	}
	else
		name = sn = newstr(cp);

	first = true;
	retval = true;

	do
	{
		char *	bp;
		int	c;

		if ( (cp = OutBox) == NULLSTR )
			break;

		if ( *name == '\0' )
			break;

		if ( (nn = strpbrk(name, seps)) != NULLSTR )
		{
			*nn++ = '\0';
			nn += strspn(nn, seps);
		}

		if ( (bp = strrchr(name, '!')) != NULLSTR && bp[1] != '\0' )
			name = bp+1;	/* Take last component of UUCP address */

		if ( (bp = strchr(name, '/')) != NULLSTR )
			*bp = '\0';	/* Peculiar address */

		for ( ep = name ; (c = *ep) != '\0' ; ep++ )
			if ( isupper(c) )
				*ep = tolower(c);

		do
		{
			if ( (ep = strchr(cp, ' ')) != NULLSTR )
				*ep++ = '\0';

			statb.st_mode = 0;

			while ( stat(cp, &statb) == SYSERROR && errno != ENOENT )
				if ( !SysWarn(CouldNot, StatStr, cp) )
				{
					retval = false;
					goto loop;
				}

			if ( (statb.st_mode & S_IFMT) == S_IFDIR )
				fn = concat(cp, Slash, name, NULLSTR);
			else
			if ( !first )
				continue;
			else
				fn = newstr(cp);

			while ( (fd = fopen(fn, "a")) == (FILE *)0 )
				if ( !SysWarn(CouldNot, WriteStr, fn) )
				{
					retval = false;
					goto next;
				}

			(void)chown(fn, Invoker.P_uid, Invoker.P_gid);
			(void)chmod(fn, 0600);

			/** UNIX header **/

			Fprintf(fd, "From %s@%s %s", LogName, SourceAddr, ctime(&Time));

			copyheader(fd, fd, true);
			if ( copybody(fd, true) )
				putc('\n', fd);
			if ( fclose(fd) == EOF )
			{
				(void)SysWarn(CouldNot, WriteStr, fn);
				retval = false;
			}
next:
			free(fn);
loop:;
		}
		while
			( (cp = ep) != NULLSTR );

		first = false;
	}
	while
		( (name = nn) != NULLSTR );

	free(sn);

	return retval;
}



#ifdef	notdef
/*
**	Find pathname for prog.
*/

char *
path(prog)
	char *		prog;
{
	register char *	pp;
	register char *	np;
	register char *	cp;
	register char *	ep;

	DODEBUG(static char tracefmt[] = "path(%s) ==> %s");

	if ( prog[0] == '/' || (ep = getenv("PATH")) == NULLSTR || ep[0] == '\0' )
	{
		Trace3(1, tracefmt, prog, prog);
		return newstr(prog);
	}

	ep = newstr(ep);

	for ( cp = ep ; cp != NULLSTR ; cp = np )
	{
		if ( (np = strchr(cp, ':')) != NULLSTR )
			*np++ = '\0';

		if ( *cp == '\0' )
			cp = ".";
		pp = concat(cp, Slash, prog, NULLSTR);

		Trace2(2, "try %s", pp);

		if ( access(pp, 1) != SYSERROR || errno == EACCES )
			break;

		errno = 0;
		free(pp);
	}

	free(ep);

	if ( cp == NULLSTR )
	{
		Trace3(1, tracefmt, prog, prog);
		return newstr(prog);
	}

	Trace3(1, tracefmt, prog, pp);

	return pp;
}
#endif	/* notdef */



/*
**	Return suitable memory increment.
*/

int
pwrof2(size)
	register int	size;
{
	register int	i = 512;

	if ( size < i )
		return i;
	if ( size > 8192 )
	{
		size |= 8191;
		return ++size;
	}
	while ( i < size )
		i <<= 1;
	return i;
}



/*
**	Print message via PAGER.
*/

void
print(header)
	bool	header;
{
	FILE *	fd;
	char *	errs;
	int	lines;
	int	window;
	ExBuf	exbuf;

	if ( (errs = getenv("LINES")) == NULLSTR || errs[0] == '\0' )
		window = 22;
	else
	if ( (window = atoi(errs)) < 3 )
		window = 22;
	else
		window -= 2;

	if ( (errs = getenv("PAGER")) == NULLSTR || errs[0] == '\0' )
		errs = Pager;

	if ( errs != NULLSTR && (lines = countlines(header)) > window )
	{
		VarArgs *vap;

		(void)signal(SIGINT, SIG_IGN);

		vap = &exbuf.ex_cmd;
		FIRSTARG(vap) = SHELL;
		NEXTARG(vap) = "-c";
		NEXTARG(vap) = errs;

		fd = (FILE *)Execute(&exbuf, allowsigint, ex_pipo, fileno(stdout));
	}
	else
	{
		lines = 0;
		fd = stdout;
	}

	if ( header )
		copyheader(fd, fd, false);
	(void)copybody(fd, false);

	if ( lines > window )
	{
		if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
			free(errs);

		(void)signal(SIGINT, catch);
	}
}



/*
**	Read body of mail, interpreting '.' commands.
*/

void
readdot()
{
	register int	c;
	register int	dot = 0;
	register bool	newline = true;

	while ( (c = getc(stdin)) != EOF )
	{
		Finished = false;

		switch ( c )
		{
		case '\n':	if ( dot )
					EOT();
				newline = true;
				break;
		case '.':	if ( dot == 0 && newline && !IgnCmds )
				{
					dot = 1;
					continue;
				}
		default:	if ( dot )
				{
					newline = dodot(c);
					dot = 0;
					continue;
				}
				newline = false;
				dot = 0;
				break;
		}

		(void)buffer(c, &Body);
	}

	if ( !newline )
		(void)buffer('\n', &Body);

	EOT();
}



/*
**	Extract net address from composite.
*/

char *
real1addr(addr)
	char *		addr;
{
	register char *	cp;
	register char *	ep;
	char		c;

	Trace2(2, "real1addr(%s)", addr);

	while ( *addr == ' ' )
		addr++;

	Trace2(4, "real1addr %s", addr);

	if ( (cp = strchr(addr, '<')) != NULLSTR )
	{
		if ( (ep = strchr(++cp, '>')) == NULLSTR )
			return newstr(cp);

		return newnstr(cp, ep-cp);
	}

	Trace2(4, "real1addr %s", addr);

	if ( (cp = strpbrk(addr, "\"(")) == NULLSTR )
		return newstr(addr);

	Trace2(4, "real1addr %s", addr);

	switch ( *cp )
	{
	case '"':	c = '"';	break;
	case '(':	c = ')';	break;
	}

	if ( (ep = strchr(cp+1, c)) == NULLSTR || (cp-addr) > (int)strlen(ep+1) )
	{
		while ( cp[-1] == ' ' )
			--cp;
		return newnstr(addr, cp-addr);
	}

	while ( *++ep == ' ' )
		;

	Trace2(4, "real1addr %s", ep);

	return newstr(ep);
}



/*
**	Extract net addr from multiple.
*/

char *
realaddr(addr)
	char *		addr;
{
	register char *	val;
	register char *	ap;
	register char *	cp	= NULLSTR;
	register char *	np;

	Trace2(2, "realaddr(%s)", addr);

	for ( ap = addr, val = NULLSTR ;; )
	{
		if ( (np = strpbrk(ap, "\"(,")) != NULLSTR )
		{
			switch ( *np )
			{
			case '"':	cp = strchr(np+1, '"');	break;
			case '(':	cp = strchr(np+1, ')');	break;
			case ',':	cp = NULLSTR;		break;
			}
			if ( cp != NULLSTR )
				np = strchr(cp+1, ',');
			if ( np != NULLSTR )
				*np = '\0';
		}

		if ( (cp = val) == NULLSTR )
			val = real1addr(ap);
		else
		{
			val = concat(cp, CommaSpace, real1addr(ap), NULLSTR);
			free(cp);
		}

		Trace2(3, "realaddr => %s", val);

		if ( (ap = np) == NULLSTR )
			break;

		*ap++ = ',';
		ap += strspn(ap, WhiteSpace);
		if ( *ap == '\0' )
			break;
	}

	Trace3(1, "realaddr(%s) => %s", addr, val);

	return val;
}



/*
**	Save mail to file.
*/

bool
save(file, append, vp)
	char *	file;
	bool	append;
	char **	vp;	/* Force if not (char **)0 */
{
	int	fd;
	FILE *	Fd;

	if ( file == NULLSTR )
	{
		Fprintf(stdout, english("(file name?)\n"));
		return false;
	}

	if ( !append || (fd = open(file, O_WRITE)) == SYSERROR )
	{
		bool	owrite;

		if ( append && errno == EACCES )
		{
			(void)SysWarn(CouldNot, WriteStr, file);
			if ( vp == (char **)0 )
				return false;
			owrite = true;
		}
		else
			owrite = false;
		if ( unlink(file) == SYSERROR && errno == EACCES )
			(void)SysWarn(CouldNot, UnlinkStr, file);
		while ( (fd = creat(file, 0600)) == SYSERROR )
			if ( !SysWarn(CouldNot, CreateStr, file) )
				return false;
		(void)chown(file, Invoker.P_uid, Invoker.P_gid);
		if ( owrite )
			Fprintf(stdout, english("Overwriting \"%s\"\n"), file);
	}
	else
		(void)lseek(fd, (long)0, 2);

	Fd = fdopen(fd, "w");

	copyheader(Fd, Fd, true);
	if ( copybody(Fd, true) )
		putc('\n', Fd);

	if ( fclose(Fd) == EOF )
	{
		(void)SysWarn(CouldNot, WriteStr, file);
		return false;
	}

	return true;
}



/*
**	Scan data for header fields and extract them,
**	returns pointer to body.
**
**	(This just throws away non-standard header lines !?)
*/

char *
scanheader(data)
	char *		data;
{
	register Env *	envp;
	register char *	cp;
	register char *	ep;
	register char *	np;
	register bool	matched;
	register char *	mdata;
	char		mhead[64];

	if ( (ep = StringMatch(data, "\n\n")) != NULLSTR )
	{
		int	len = ep - data;

		mdata = Malloc(len+2);
		(void)strncpy(&mdata[1], data, len);
		mdata[len+1] = '\0';
		ep += 2;
	}
	else
	{
		mdata = Malloc(strlen(data)+2);
		(void)strcpy(&mdata[1], data);
	}

	Trace2(1, "scanheader(%s)", &mdata[1]);

	mhead[0] = '\n';
	mdata[0] = '\n';

	for ( matched = false, envp = Envs ; envp < &Envs[NENVS] ; envp++ )
	{
		if ( !envp->out )
			continue;

		if ( matched && envp->p != &ToAddr )
		{
			FreeStr(envp->p);
			envp->fixed = false;
		}

		(void)strcpy(&mhead[1], envp->head);	/* Preceded by newline */

		if ( (cp = StringCMatch(mdata, mhead)) == NULLSTR )
			continue;

		matched = true;

		/** Find end of line (include continuation lines) **/

		np = ++cp;
		do
			if ( (np = strchr(++np, '\n')) == NULLSTR )
				break;
		while
			( np[1] == ' ' || np[1] == '\t' );
		if ( np != NULLSTR )
			*np = '\0';

		if ( envp->p != &Date )
		{
			cp += strlen(mhead+1);

			Trace4(
				2,
				"replace %s\"%s\" with \"%s\"",
				envp->head,
				(*(envp->p)==NULLSTR)?EmptyString:*(envp->p),
				cp
			);

			*(envp->p) = newstr(cp);
			envp->fixed = false;
		}

		if ( np != NULLSTR )
			*np = '\n';
	}

	free(mdata);

	if ( ep != NULLSTR || matched )
	{
		if ( ep == NULLSTR )
			ep = data + strlen(data);

		Trace3(2, "scanheader ==> %.*s", 16, ep);
		return ep;
	}

	Trace1(2, "scanheader ==> <null header>");
	return data;	/* No header */
}



/*
**	Invoke network to send mail.
**
**	(Destroys `to'.)
*/

void
sendmsg(to, bcc)
	register char *	to;
	bool		bcc;
{
	register char *	cp;
	FILE *		fd;
	VarArgs *	vap;
	char		tbuf[NUMERICARGLEN];
	ExBuf		exbuf;

	Trace3(1, "sendmsg(%s, %sbcc)", to, bcc?EmptyString:"no");

	if ( Sender == NULLSTR )
		Sender = concat(SPOOLDIR, LIBDIR, SENDER, NULLSTR);

	vap = &exbuf.ex_cmd;

	while ( to != NULLSTR )
	{
		FIRSTARG(vap) = Sender;

		if ( Ack )
			(void)SplitArg(vap, SendAckARGS);
		else
			(void)SplitArg(vap, SendARGS);

		if ( Ack || ReportID )
		{
			NEXTARG(vap) = "-q";	/* Assume this works! */
			if ( !ReportID )
				Fprintf(stdout, english("%sessage identifier: "), bcc?"BCC m":"M");
		}

		if ( NoRet )
			NEXTARG(vap) = "-r";	/* Assume this works! */

		if ( Traceflag )
			NEXTARG(vap) = NumericArg(tbuf, 'T', (Ulong)Traceflag);

		for ( ;; )
		{
			if ( (cp = strpbrk(to, CommaSpace)) != NULLSTR )
			{
				*cp++ = '\0';
				cp += strspn(cp, CommaSpace);
				if ( *cp == '\0' )
					cp = NULLSTR;
			}

			NEXTARG(vap) = concat("-Q", to, NULLSTR);

			if ( (to = cp) == NULLSTR || NARGS(vap) >= MAXVARARGS )
				break;
		}

		(void)fflush(stdout);

		fd = (FILE *)Execute(&exbuf, setarg0, ex_pipo, fileno(stdout));

		copyheader(fd, bcc?fd:(FILE *)0, false);
		(void)copybody(fd, true);

		if ( ExClose(&exbuf, fd) != NULLSTR )
			catch(SIGINT);
	}
}



/*
**	Function called from Execute() -- replace ARG(0) with Name.
*/

void
setarg0(vap)
	VarArgs *	vap;
{
	ARG(vap, 0) = ParentName;
}



/*
**	Restore `stdin' for elfic progs.
*/

void
setin(vap)
	VarArgs *	vap;
{
	(void)close(0);
	(void)dup(1);
	allowsigint(vap);
}



/*
**	Find "match" in "string" - case insensitive.
**	Return pointer to start of match, or NULLSTR.
**
**	`QuickSearch' algorithm.
**	[D.M.Sunday, CACM Aug '90, Vol.3 No.8 pp.132-142]
*/

#define	ASIZE	040
#define	AMASK	(ASIZE-1)

char *
StringCMatch(string, match)
	register char *	string;
	register char *	match;
{
	register int	i;
	register int	j;
	register int *	a;
	register char *	m;
	register char *	s;
	int		b[ASIZE];

	if ( string == NULLSTR || match == NULLSTR )
		return NULLSTR;

	Trace3(5, "StringCMatch(%.22s, %.22s)", string, match);

	for ( m = match, s = string ; *m++ != '\0' ; )
		if ( *s++ == '\0' )
			return NULLSTR;

	if ( (i = m - match) == 1 )
	{
		if ( string[0] == '\0' )
			return string;
		else
			return NULLSTR;
	}

	/** `i' is length of `match' +1 **/

	for ( a = b, j = ASIZE ; --j >= 0 ; )
		*a++ = i;

	for ( a = b, m = match ; --i > 0 ; )
		a[(*m++)&AMASK] = i;

	j = m - match;

	/** `j' is length of `match' **/

	for ( ;; )
	{
		for ( m = match, s = string ; (i = *m++) ; )
			if ( (i|040) != (*s++|040) )
				break;

		if ( i == '\0' )
		{
			Trace3(4, "StringCMatch(%.22s, %.22s) MATCH", string, match);
			return string;
		}

		s = string + j;

		i = a[(*s)&AMASK];

		for ( string += i ; --i >= 0 ; )
			if ( *s++ == '\0' )
				return NULLSTR;
	}
}



/*
**	Own version of `StripEnv()' -- default sanitises `environ'.
*/

char **
StripEnv()
{
	extern char **	environ;

	Trace2(1, "StripEnv() ==> PATH=%s", getenv("PATH"));
	return environ;
}



/*
**	Change acknowledgment status.
*/

void
toggleAck(set)
	bool	set;
{
	char *	o;

	if ( Ack == false || set )
	{
		Ack = true;
		o = english("on");
	}
	else
	{
		Ack = false;
		o = english("off");
	}

	Fprintf(stdout, english("(acknowledgement %s)\n"), o);
}



/*
**	`uuencode' file data into message buffer.
*/

void
uuencode(file, data)
	char *		file;
	char *		data;
{
	register char *	cp;
	register int	c1;
	register int	c2;
	register int	c3;
	register int	j;
	register int	i;
	char *		buf;

	static char	fmt1[]	= "begin %o %s\n";
	static char	fmt2[]	= "end\n";

	extern int	RdFileMode;

	Trace3(1, "uuencode(%s, %#lx)", file, (PUlong)data);

	j = strlen(fmt1)-4 + 3 + strlen(file) + 1;	/* +3 for mode +1 for '\0' */

	/** Advise size to buffer() **/

	i = RdFileSize;
	NewLen = ((i+2)/3)*4;				/* Every 3 expands to 4 */
	NewLen += (i+UUINLINESIZE-1)/UUINLINESIZE;	/* \n per UUINLINESIZE */
	NewLen += j + 4;				/* begin + end */

	Trace6(3, "uuencode: i=%d, j=%d, NewLen=%d, ((i+2)/3)*4=%d, (i+UUINLINESIZE-1)/UUINLINESIZE=%d", i, j, NewLen, ((i+2)/3)*4, (i+UUINLINESIZE-1)/UUINLINESIZE);

	buf = cp = Malloc(j);
	(void)sprintf(cp, fmt1, RdFileMode & 0777, file);

	while ( (c1 = *cp++) != '\0' )
		(void)buffer(c1, &Body);

	free(buf);

	cp = data;
	cp[i] = '\n';		/* ReadFile() truncates last '\n' (if any) */
	cp[++i] = '\0';		/* And guarantees 2 extra bytes */

	while ( i > 0 )
	{
		if ( (j = i) > UUINLINESIZE )
			j = UUINLINESIZE;
		i -= j;
		(void)buffer(UUTable[j], &Body);
		for ( ; j > 0 ; j -= 3 )
		{
			c1 = *cp++;
			c2 = *cp++;
			c3 = *cp++;
			(void)buffer(UUTable[  (c1>>2)&077 ], &Body);
			(void)buffer(UUTable[ ((c1<<4)&060) | ((c2>>4)&017) ], &Body);
			(void)buffer(UUTable[ ((c2<<2)&074) | ((c3>>6)&003) ], &Body);
			(void)buffer(UUTable[  (c3   )&077 ], &Body);
		}
		(void)buffer('\n', &Body);
	}

	(void)buffer(UUTable[0], &Body);	/* `uuencode' always requires final 0! */
	(void)buffer('\n', &Body);

	for ( cp = fmt2 ; (c1 = *cp++) != '\0' ; )
		(void)buffer(c1, &Body);
}
