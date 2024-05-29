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


#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"sysexits.h"

#include	"Netinit.h"
#include	"net.h"

#include	<ctype.h>
#include	<time.h>


char *		cmd;
char *		comp_pat;
char *		initargs;
int		MaxIdLen;
int		MaxCronLen;
int		netPid;
char *		outfile;
char *		re;
bool		Toolong;

static void	ChangeStatus _FA_((FILE *, int, char *));
int		cmdcmp _FA_((const void *, const void *));
static void	donewlog _FA_((FILE *));
static void	dostatus _FA_((FILE *, bool));
static void	getstr _FA_((FILE *, char **));
static bool 	match _FA_((char *));
static void	perform _FA_((Cmd *, FILE *));
char *		plural _FA_((char, Ulong));

#define	Fprintf	(void)fprintf



void
NetMessage()
{
	register FILE *	fd;
	register Cmd *	cmdp;

	NameProg(english("%s COMMAND"), Name);

	if ( (fd = fopen(initargs, "r")) == (FILE *)0 )
	{
		(void)SysWarn(CouldNot, OpenStr, initargs);
		netsignal = 0;
		(void)signal(SIGWAKEUP, NetSig);
		return;
	}

	Toolong = false;

	getstr(fd, &outfile);
	getstr(fd, &cmd);	netPid = atoi(cmd);
	getstr(fd, &cmd);
	getstr(fd, &re);

	(void)fclose(fd);

	MesgN(english("command"), "\"%s %s\"", cmd, re);

	if ( Toolong )
		cmdp = (Cmd *)0;
	else
		cmdp = (Cmd *)bsearch(cmd, (char *)CmdTab, (int)NCmds, sizeof(Cmd), cmdcmp);

	if ( cmdp == (Cmd *)0 || cmdp->type != shutdown_cmd )
		for ( ;; )
		{
			switch ( fork() )
			{
			case SYSERROR:
				if ( errno == ENOMEM )
				{
					SysWarn(CouldNot, ForkStr, cmd);
					(void)sleep(20);
				}
				else
					Syserror(CouldNot, ForkStr, cmd);
				continue;
			case 0:
				ExInChild = true;
				break;
			default:
				perform(cmdp, (FILE *)0);
				(void)sleep(MINSLEEP+1);
				netsignal = 0;
				(void)signal(SIGWAKEUP, NetSig);
				return;
			}
			break;
		}

	/*
	**	In child, or "shutdown".
	*/

	if ( (fd = fopen(outfile, "w")) == (FILE *)0 )
	{
		(void)SysWarn(CouldNot, WriteStr, outfile);
		fd = ErrorFd;
	}

	perform(cmdp, fd);
	/* No return */
	exit(EX_SOFTWARE);
}



/*
**	Command name compare for bsearch.
*/

int
#ifdef	ANSI_C
cmdcmp(const void *cp, const void *cmdp)
#else	/* ANSI_C */
cmdcmp(cp, cmdp)
	char *	cp;
	char *	cmdp;
#endif	/* ANSI_C */
{
	return strccmp((char *)cp, ((Cmd *)cmdp)->name);
}



/*
**	Read next line into new string.
*/

static void
getstr(fd, str)
	register FILE *	fd;
	char **		str;
{
	register char *	cp;
	register int	c;
	char		buf[256];

	for ( cp = buf ; ; )
	{
		if ( (c = getc(fd)) == EOF || c == '\n' )
			break;
		if ( cp >= &buf[sizeof buf] )
		{
			Toolong = true;
			break;
		}
		*cp++ = c;
	}

	*cp++ = '\0';

	FreeStr(str);
	*str = cp = Malloc(cp-buf);
	(void)strcpy(cp, buf);
}



static void
perform(cmdp, fd)
	register Cmd *	cmdp;
	register FILE *	fd;
{
	if ( cmdp == (Cmd *)0 )
	{
		if ( fd == (FILE *)0 )
			return;

		if ( Toolong )
			Fprintf(fd, english("Overlong command ignored.\n"));
		else
			Fprintf(fd, english("\"%s\" is not a recognised command.\n"), cmd);
	}
	else
	if ( re[0] != '\0' && (comp_pat = regcmp(re, NULLSTR)) == NULLSTR )
	{
		if ( fd == (FILE *)0 )
			return;

		Fprintf(fd, english("cannot compile pattern \"%s\"\n"), re);
	}
	else
	if ( re[0] == '\0' && cmdp->regx )
	{
		if ( fd == (FILE *)0 )
			return;

		Fprintf(fd, english("\"%s\" needs a pattern\n"), cmdp->name);
	}
	else
	switch ( cmdp->type )
	{
	case status_cmd:
		dostatus(fd, false);
		break;
	case statusv_cmd:
		dostatus(fd, true);
		break;
	case scan_cmd:
		doscan(fd);
		break;
	case newlog_cmd:
		donewlog(fd);
		break;
	case traceoff_cmd:
		Traceflag = 0;
		break;
	case traceon_cmd:
#		if	DEBUG
		if ( re[0] >= '0' && re[0] <= '9' )
			Traceflag = atoi(re);
		else
			Traceflag++;
		if ( fd != (FILE *)0 )
			Fprintf(fd, english("%s: trace level %d\n"), Name, Traceflag);
#		endif	/* DEBUG */
		break;
	case stop_cmd:
		ChangeStatus(fd, TERMINATE, english("stopped"));
		break;
	case kill_cmd:
		ChangeStatus(fd, KILL, english("killed"));
		break;
	case curtail_cmd:
		ChangeStatus(fd, DISABLED, english("curtailed"));
		break;
	case start_cmd:
		ChangeStatus(fd, ENABLED, english("started"));
		break;
	case shutdown_cmd:
		Fprintf(fd, english("%s: stopping processes...\n"), Name);
		netshutdown();
		Fprintf(fd, english("%s: terminated.\n"), Name);
	}

	if ( fd == (FILE *)0 )
		return;

	(void)fclose(fd);
	(void)sleep(MINSLEEP);
	(void)kill(netPid, SIGWAKEUP);	/* Tell net to finish */
	finish(EX_OK);
}



static void
donewlog(fd)
	FILE *	fd;
{
	extern char *	LogName;
	static char	mesg[] = english("New log file \"%s\"\n");

	if ( fd != (FILE *)0 )
	{
		Fprintf(fd, mesg, LogName);
		return;
	}

	StderrLog(LogName);	/* Close old, open new */

	Report(mesg, LogName);
}



static void
dostatus(fd, verbose)
	register FILE *	fd;
	bool		verbose;
{
	register Proc *	pp;
	register char *	cp;
	register bool	print_start;
	char *		buf;
	char		dateb[DATETIMESTRLEN];
	char		timeb[TIMESTRLEN];

	if ( fd == (FILE *)0 )
		return;

	buf = Malloc(MaxCronLen+3);

	if ( verbose )
	{
		Fprintf
		(
			fd,
			english("Network started at %s, up %s\n"),
			DateTimeStr(StartTime, dateb),
			TimeString(Time-StartTime, timeb)
		);

		if ( re[0] != '\0' )
			Fprintf
			(
				fd,
				english("Status of \"%s\":\n\n"),
				re
			);
		else
			Fprintf
			(
				fd,
				english("Status of all processes:\n\n")
			);
	}

	for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
	{
		if ( !match(pp->procId) )
			continue;

		print_start = true;

		Fprintf(fd, "%-*s", MaxIdLen, pp->procId);

		switch ( pp->type )
		{
		case RUNONCE:	cp = english("runonce");
				print_start = false;
				break;
		case RESTART:	cp = english("restart");
				break;
		case CRON:	cp = pp->crontime->string;
				if ( pp->status == DISABLED )
					print_start = false;
				break;
		default:	cp = english("???");
		}
		(void)sprintf(buf, "(%s)", cp);
		Fprintf(fd, " %-*s", MaxCronLen+2, buf);

		if ( !verbose )
		{
			switch ( pp->status )
			{
			case DISABLED:	if
					(
						pp->type == RESTART
						||
						(pp->type == RUNONCE && pp->pid == NOTRUNNING)
					)
					{
						cp = english("disabled");
						print_start = false;
						break;
					}
			case ENABLED:	cp = english("enabled");
					break;
			default:	cp = english("terminating");
					print_start = false;
					break; 
			}

			Fprintf(fd, " %-11s", cp);
		}

		if ( pp->pid == NOTRUNNING )
		{
			if ( verbose )
				Fprintf(fd, english(" is not running"));
		}
		else
		{
			if ( verbose )
				Fprintf(fd, english(" is running [pid: %d]"), pp->pid);
			else
				Fprintf(fd, english(" [%d]"), pp->pid);
			print_start = false;
		}

		if ( (pp->restarts > 0 && pp->type != RESTART) || pp->restarts > 1 || pp->errors > 0 )
		{
			if ( verbose )
				Fprintf(fd, english("\nand has logged "));
			else
				Fprintf(fd, " (");
			if ( (pp->restarts > 0 && pp->type != RESTART) || pp->restarts > 1 )
			{
				if ( pp->type == RESTART )
					Fprintf
					(
						fd,
						english("%lu restart%s"),
						(PUlong)pp->restarts-1,
						plural('t', pp->restarts-1)
					);
				else
					Fprintf
					(
						fd,
						english("%lu activation%s"),
						(PUlong)pp->restarts,
						plural('n', pp->restarts)
					);
				if ( pp->errors > 0 )
					Fprintf(fd, ", ");
			}
			if ( pp->errors > 0 )
				Fprintf
				(
					fd,
					english("%lu error%s"),
					(PUlong)pp->errors,
					plural('r', pp->errors)
				);
			if ( !verbose )
				Fprintf(fd, ")");
		}

		if ( !verbose )
		{
			if ( print_start )
			{
				Ulong	st = pp->start + pp->delay;

				if ( st > Time )
					Fprintf(fd, english(" starting in %s"), TimeString(st-Time, timeb));
				else
					Fprintf(fd, english(" starting"));
			}

			putc('\n', fd);
			continue;
		}

		Fprintf(fd, english(".\nThis process is "));

		switch ( pp->status )
		{
		case DISABLED:	if ( pp->type != CRON )
				{
					cp = english("disabled");
					print_start = false;
					break;
				}
		case ENABLED:	cp = english("enabled");
				break;
		case TERMINATE:	cp = english("to be terminated");
				print_start = false;
				break; 
		default:	cp = english("to be killed");
				print_start = false;
				break;
		}
		Fprintf(fd, "%s and ", cp);

		if ( pp->start == 0 )
		{
			Fprintf(fd, english("has not yet run.\n"));
			print_start = false;
		}
		else
			Fprintf(fd, english("was last started on %s"), ctime(&pp->start));

		if ( print_start )
		{
			time_t	start_time = pp->start + pp->delay;

			Fprintf(fd, english("and will next be started on %s"), ctime(&start_time));
		}

		putc('\n', fd);
	}

	free(buf);
}



void
doscan(fd)
	FILE *	fd;
{
	Proc *	op;
	Proc *	np;
	Proc *	OldProcs;

	if ( fd != (FILE *)0 )
	{
		Fprintf(fd, english("Scanning \"%s\"\n"), InitFile);
		return;
	}

	MesgN(english("rescan"), "\"%s\"", InitFile);

	MaxIdLen = 0;
	MaxCronLen = 0;
	
	OldProcs = Procs;
	Procs = (Proc *)0;
	parse();

	for ( op = OldProcs; op != (Proc *)0; op = op->next )
	{
		for ( np = Procs; np != (Proc *)0; np = np->next )
			if ( strcmp(op->procId, np->procId) == STREQUAL )
				break;

		if ( np == (Proc *)0 )
		{
			int	len;

			/** Old process not found in new list so... **/
			op->status = REMOVE;
			if ( op->pid != NOTRUNNING )
				(void)kill(op->pid, SIGKILL);
			if ( (len = strlen(op->procId)) > MaxIdLen )
				MaxIdLen = len;
			if ( op->type == CRON && (len = strlen(op->crontime->string)) > MaxCronLen )
				MaxCronLen = len;
		}
		else
		{
			np->start = op->start;
			np->restarts = op->restarts;
			np->errors = op->errors;
			np->status = op->status;
			np->pid = op->pid;
			np->delay = op->delay;
			np->check = op->check;

			op->status = DELETE;
		}
	}

	/* now delete or merge entries from old list */
	for ( op = OldProcs; op != (Proc *)0; )
	{
		np = op->next;
		if ( op->status == DELETE )
		{
			if ( op->crontime != (Cron *)0 )
			{
				free(op->crontime->string);
				free((char *)op->crontime);
			}
			free(op->command);
			free((char *)op);
		}
		else
		{
			op->next = Procs;
			Procs = op;
		}
		op = np;
	}
}



static void
ChangeStatus(fd, s, act)
	FILE *	fd;
	int	s;
	char *	act;
{
	Proc *	pp;

	if ( fd != (FILE *)0 )
		Fprintf(fd, english("The following processes will be %s: "), act);

	for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
		if ( match(pp->procId) )
		{
			if ( fd != (FILE *)0 )
				Fprintf(fd, "%s ", pp->procId);
			pp->status = s;
		}

	if ( fd != (FILE *)0 )
		putc('\n', fd);
}



static bool
match(s)
	char *	s;
{
	if ( re[0] == '\0' )
		return true;

	if ( regex(comp_pat, s, NULLSTR) != NULLSTR )
		return true;

	return false;
}



/*
**	Return plural ending of a noun if ``num'' != 1.
*/

char *
#ifdef	ANSI_C
plural(char suffix, Ulong num)
#else	/* ANSI_C */
plural(suffix, num)
	char	suffix;
	Ulong	num;
#endif	/* ANSI_C */
{
	switch ( suffix )
	{
	case 'y':
		return num == 1 ? english("y") : english("ies");
	case 'e':
		return num == 1 ? english("e") : english("es");
	default:
		return num == 1 ? EmptyString : english("s");
	}
}
