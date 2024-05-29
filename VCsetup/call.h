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
**	`Builtin' patterns.
*/

#define	DEVFAIL		"-FAIL-"
#define	DEVLOCKED	"-LOCKED-"
#define	DEVOK		"-OK-"
#define EOFSTR		"-EOF-"
#define	TERMINATE	"-TERMINATE-"
#define	TIMEOUT		"-TIMEOUT-"
#define UNDEFINED	"-UNDEFINED-"

/*
**	Global defines.
*/

#define	MAX_LINE_LEN	400
#define	MAXCALL		16

#define ERROR		1
#define OK		0
#define UNDEF		-1

#undef	SHELL

/*
**	Structure declarations.
*/

union operand
{
	char		*str; 
	long		num;
	struct	Symbol	*sym;
	struct	Pat	*pat;
	struct	Op	*op;
	struct	Vlist	*vlist;
};

typedef struct Op
{
	struct	Symbol	*lab;		/* pointer to label for this op */
	int		code;		/* opcode for this operation */
	union operand	op1;		/* first operand */
	union operand	op2;		/* second operand */
	struct 	Op	*next;
} Op;

typedef struct Symbol 
{
	char		*name;
	int		type;		/* type of symbol */
	int		vtype;		/* type of value */
	union operand	val;		/* value: string, number or operation */
	struct Symbol	*great;		/* to link to > symbol */
	struct Symbol	*less;		/* to link to < symbol */
} Symbol;
			
typedef struct Pat
{
	char		*pattern;	/* pattern string */
	char		*comp_pat;	/* compiled pattern */
	struct	Symbol	*nextsym;
	struct 	Pat	*next;
} Pat;

typedef struct Vlist
{
	struct Symbol	*sym;		/* this symbol */
	struct Vlist	*next;		/* link to next variable */
	struct Vlist	*prev;		/* link to previous variable */
} Vlist;

typedef enum
{
	vc_open, vc_device, vc_close
} DevOp;

/* Redefine old definitions (used by X/Open Transport Interface) */
#define	t_open		vc_open
#define	t_device	vc_device
#define	t_close		vc_close

typedef struct
{
	char *	devname;
	char *	(*func)_FA_((DevOp, VarArgs *));
} Device;

typedef struct
{
	Op *	head;
	int	lineno;
	char *	name;
	FILE *	fd;
} CallFrame;


#if	XTI == 1
extern int	t_snd();
extern bool	xti_connect;
#endif


/*
**	Global variables.
*/
			
extern Op *		AfterList;
extern SigFunc		alrmcatch;
extern char *		callargs;
extern int		calldepth;
extern bool		CallErr;
extern CallFrame	CallScript;
extern bool		Cookflag;
extern char *		delim;
extern Symbol *		DELIMsym;
extern Device		DevFuns[];
extern int		Devsz;
extern int		Fd;
extern bool		Finish;
extern char *		Fn;
extern char *		HomeName;
extern char		input[];
extern Symbol *		lexsym;
extern char *		LinkDir;
extern int		Monitorflag;
extern int		NDevs;
extern char		NoDevice[];
extern int		Ondelay;
extern char *		Reason;
extern Symbol *		RESULTsym;
extern int		Retries;
extern char *		SLinkName;
extern unsigned int	TimeOut;
extern int		UUCPlocked;
extern int		VCtype;
extern bool		XonXoff;

extern char *		command_name _FA_((int));
extern int		ConnectSocket _FA_((char *, char *, char *, int, bool, char *));
extern int		do_daemon _FA_((char *));
extern int		do_execd _FA_((char *));
extern int		do_forkc _FA_((char *));
extern int		do_forkd _FA_((char *));
extern int		do_mode _FA_((char *));
extern char *		do_shell _FA_((char *));
extern int		do_speed _FA_((char *));
extern char *		do_stty _FA_((char *));
extern int		do_slowwrite _FA_((char *));
extern int		do_write _FA_((char *));
extern void		getinput _FA_((char *));
extern void		InitInterp _FA_((void));
extern Symbol *		install _FA_((char *, int));
extern Symbol *		InstallId _FA_((char *, char *));
extern int		interp _FA_((Op *, char *));
extern char *		ip_close _FA_((void));
extern char *		ip_device _FA_((VarArgs *));
extern Symbol *		lookup _FA_((char *));
extern Op *		MakeCall _FA_((char *));
extern bool		match _FA_((char *, Pat *));
extern Op *		op _FA_((int));
extern Pat *		pat _FA_((Symbol *, Symbol *));
extern char *		tty_device _FA_((VarArgs *));
extern Vlist *		vlelt _FA_((Symbol *));
extern void		warning _FA_((char *, char *));
extern int		yyparse _FA_((void));
