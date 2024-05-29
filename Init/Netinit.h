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


#define	MAX_LINE_LEN	512
#define	CMDSIZE		512
#define	MAXLIST		63
#define	NOTRUNNING	SYSERROR

#define	RUNONCE		0	/* Types */
#define	RESTART		1
#define	CRON		2

#define REMOVE		-2	/* Stati */
#define DELETE		-1
#define ENABLED		0
#define DISABLED	1
#define TERMINATE	2
#define KILL		3

#define	MAXDELAY	(6*60)	/* Longest delay between execs */
#define	MINDELAY	20	/* Minimum delay between execs */
#define	CRONDELAY	60	/* Delay between execs of cron jobs */
#define	WAITINCR	60	/* Increment to delay if quick termination */
#define	MINWAIT		2	/* Shortest wait allowed */

#define NAME		257
#define NSTRING		258
#define END	 	259

typedef struct Timespec
{
	enum {any_t, range_t, list_t, exact_t, err_t} type;
	int	val[MAXLIST];
} Timespec;

typedef struct Cron
{
	Timespec	min;
	Timespec	hour;
	Timespec	mday;
	Timespec	month;
	Timespec	wday;
	char *		string;
} Cron;

/*
**	Structure and list to hold active processes.
*/

typedef struct Proc	Proc;
struct Proc
{
	Proc *	next;
	char *	procId;
	char *	command;
	Cron *	crontime;
	Time_t	start;
	Ulong	restarts;
	Ulong	errors;
	short	type;
	short	status;
	int	pid;
	int	delay;
	int	check;
};

/*
**	Structure for process type.
*/

typedef struct
{
	char *	name;
	int	type;
}
		Type;

Extern Type	TypTab[]
#ifdef	INIT_DATA
=
{
	{english("restart"),	RESTART},
	{english("runonce"),	RUNONCE},
	{NULLSTR,		CRON}
}
#endif	/* INIT_DATA */
;

/*
**	Structure for command type.
*/

typedef enum
{
	curtail_cmd,
	kill_cmd,
	scan_cmd,
	shutdown_cmd,
	start_cmd,
	status_cmd,
	statusv_cmd,
	stop_cmd,
	traceon_cmd,
	traceoff_cmd,
	newlog_cmd
}
		CmdTyp;

typedef struct
{
	char *	name;
	CmdTyp	type;
	bool	regx;	/* <regex> needed */
}
		Cmd;

/**	english("SORT THIS ARRAY")	**/

Extern Cmd	CmdTab[]
#ifdef	INIT_DATA
=
{
	{english("curtail"),	curtail_cmd,	true},
	{english("kill"),	kill_cmd,	true},
	{english("newlog"),	newlog_cmd,	false},
	{english("scan"),	scan_cmd,	false},
	{english("shutdown"),	shutdown_cmd,	false},
	{english("start"),	start_cmd,	true},
	{english("status"),	status_cmd,	false},
	{english("statusv"),	statusv_cmd,	false},
	{english("stop"),	stop_cmd,	true},
	{english("traceoff"),	traceoff_cmd,	false},
	{english("traceon"),	traceon_cmd,	false},
	{english("vstatus"),	statusv_cmd,	false},
}
#endif	/* INIT_DATA */
;

#ifdef	INIT_DATA
#define	NCMDS	((sizeof(CmdTab))/(sizeof(Cmd)))
#endif	/* INIT_DATA */

Extern int	NCmds
#ifdef	INIT_DATA
=		NCMDS
#endif	/* INIT_DATA */
;

Extern Time_t	StartTime;

extern bool	ExInChild;
extern char *	initargs;
extern char *	InitFile;
extern int	lexlen;
extern char *	lexval;
extern char *	LockName;
extern int	MaxIdLen;
extern int	MaxCronLen;
extern int	netsignal;
extern int	Nlinks;
extern int	NveTimeDelay;
extern int	opid;
extern Proc *	Procs;
extern int	ProgsActive;
extern Time_t	ScanDate;
extern int	woken;

extern SigFunc	catchalarm;
extern int	cron _FA_((struct tm *, Proc *));
extern void	doscan _FA_((FILE *));
extern int	lex _FA_((void));
extern void	mkcron _FA_((Proc *, char *));
extern Proc *	mkproc _FA_((char *, char *, char *));
extern void	NetMessage _FA_((void));
extern SigFunc	NetSig;
extern char *	Realloc _FA_((char *, int));
extern bool	Run1Prog _FA_((Proc *));
extern void	RunProgs _FA_((void));
extern void	netshutdown _FA_((void));
extern void	parse _FA_((void));
extern SigFunc	termin;
extern void	warning _FA_((char *));
