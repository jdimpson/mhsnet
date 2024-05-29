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


#define	STAT_CALL
#define	STDIO

#include "global.h"
#include "debug.h"
#include "Netinit.h"


Proc *	Procs;
FILE *	fin;

#ifdef	DEBUG
static void	DumpProcs _FA_((Proc *));
#endif	/* DEBUG */



void
parse()
{
	register char *	cmdp;
	register int	tok;
	Proc *		p;
	int		totsize;
	int		cmdsize;
	char *		id;
	char *		cmd;
	char *		t;
	struct stat	statb;

	NameProg(english("%s SCANNING %s"), Name, InitFile);

	totsize = CMDSIZE;
	cmd = Malloc(totsize);

	while ( (fin = fopen(InitFile, "r")) == NULL )
		Syserror(CouldNot, OpenStr, InitFile);

	if ( fstat(fileno(fin), &statb) != SYSERROR )
		ScanDate = statb.st_mtime;

	while ( (tok = lex()) )
	{
		if ( tok == END )
			continue; /* ignore extra newlines */

		if ( tok != NAME )
		{
			warning("process-id expected");
			continue;
		}
		id = lexval;
		lexval = NULLSTR;
		if ( lexlen > MaxIdLen )
			MaxIdLen = lexlen;

		if ( ((tok = lex()) != NAME) && (tok != NSTRING) )
		{
			free(id);
			warning("type expected");
			continue;
		}
		t = lexval;
		lexval = NULLSTR;
		if ( lexlen > MaxCronLen )
			MaxCronLen = lexlen;

		cmdsize = totsize-6;
		cmdp = strcpyend(cmd, "exec");

		while ( (tok = lex()) != END )
		{
			if ( tok == 0 )
			{
				warning("EOF unexpected");
				break;
			}

			if ( cmdsize < (lexlen+2) )
			{
				char *	ncmd = Malloc(totsize += CMDSIZE);

				cmdp = strcpyend(ncmd, cmd);
				free(cmd);
				cmd = ncmd;
				cmdsize += CMDSIZE;
			}

			*cmdp++ = ' ';

			if ( tok == NSTRING )
			{
				*cmdp++ = '"';
				cmdp = strcpyend(cmdp, lexval);
				*cmdp++ = '"';
				*cmdp = '\0';
				cmdsize -= 2;
			}
			else
				cmdp = strcpyend(cmdp, lexval);

			cmdsize -= lexlen + 1;
		}

		if ( Procs == (Proc *)0 )
			Procs = p = mkproc(id, t, newstr(cmd));
		else
		{
			p->next = mkproc(id, t, newstr(cmd));
			p = p->next;
		}
	}

	free(cmd);
	(void)fclose(fin);

	DODEBUG(if(Traceflag)DumpProcs(Procs));
}



/*
**	build item record
*/

Proc *
mkproc(id, tp, cmd)
	char *		id;
	char *		tp;
	char *		cmd;
{
	register Proc *	p;
	register Type *	t;

	Trace4(2, "mkproc(%s, %s, %s)", id, tp, cmd);

	p = Talloc0(Proc);
	p->procId = id;
	p->command = cmd;
	p->start = 0;
	p->restarts = 0;
	p->errors = 0;
	p->status = DISABLED;
	p->delay = MINDELAY;
	p->pid = NOTRUNNING;	/* temp */

	for ( t = TypTab ; t->name != NULLSTR ; t++ )
		if ( strccmp(tp, t->name) == STREQUAL )
		{
			p->type = t->type;
			if ( p->type == RESTART )
				p->status = ENABLED;
			free(tp);
			return p;
		}

	mkcron(p, tp);

	p->type = CRON;
	return p;
}



#if	DEBUG
static void
DumpProcs(pp)
	register Proc *	pp;
{
	for ( ; pp != (Proc *)0 ; pp = pp->next )
		Trace(1, ">> %-*s \"%s\"", MaxIdLen, pp->procId, pp->command);
}
#endif	/* DEBUG */
