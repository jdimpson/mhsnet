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


/**	IF STATE INPUT IS ALL BUFFERRED, IF Lookup() FAILS, MUST PASS newstr()	**/

/*
**	Read commands from file and update state information in place.
*/

#define	STDIO
#define	COMMAND_DATA

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"statefile.h"
#include	"commands.h"

#include	<setjmp.h>


/*
**	`_getline()' breaks out tokens.
*/

#define	LINE_TOKENS	5

static char *		BufToks[LINE_TOKENS];
static char *		BufBrks[LINE_TOKENS];

/*
**	Miscellaneous
*/

static FILE *		CmdsFd;		/* Commands file */
static int		Line;		/* Line number in commands file */


int			cmpcom _FA_((const void *, const void *));
static int		_getline _FA_((void));

extern char		_ERROR_[];
extern int		CTraceflag;
extern jmp_buf		NameErrJmp;
extern bool		NoJmp;



/*
**	Parse command line
*/

bool
Rcommands(fd, warn)
	FILE *			fd;
	bool			warn;
{
	register Cmdp		cmdp;
	register char *		cp;
	register Entry *	ep1;
	register Entry *	ep2;
	register NLink *	lp;
	register char *		errp;
	register int		token;
	bool			ojmp;
	bool			retval = true;

	static Cmdp		last_cmd;
	auto char *		aerrp;

	char			buf1[TOKEN_SIZE+1];
	char			buf2[TOKEN_SIZE+1];

	Command			null_cmd;

	static char	errfmt[]	= english("%s in command \"%s\" at line %d");
	static char	missing_link[]	= english("missing link");

	if ( CTraceflag > 0 )
		Traceflag = CTraceflag;

	Trace3(1, "Rcommands(%d,%s)", fileno(fd), warn?" warn":EmptyString);

	CmdsFd = fd;
	IsCommand = true;
	Line = 0;
	null_cmd.cd_name = EmptyString;
	last_cmd = &null_cmd;
	ep2 = (Entry *)0;	/* gcc -Wall */

	switch ( (NameErr)setjmp(NameErrJmp) )
	{
	case ne_ok:		break;
	default:		Fatal1(english("unrecognised NameErr"));
	case ne_floor:		errp = Ne_floor;	goto error;
	case ne_illc:		errp = Ne_illc;		goto error;
	case ne_illtype:	errp = Ne_illtype;	goto error;
	case ne_noname:		errp = Ne_noname;	goto error;
	case ne_notype:		errp = Ne_notype;	goto error;
	case ne_null:		errp = Ne_null;		goto error;
	case ne_toolong:	errp = Ne_toolong;	goto error;
	case ne_unktype:	errp = Ne_unktype;	goto error;

error:			Error(errfmt, errp, last_cmd->cd_name, Line);
			retval = false;
	}

	while ( _getline() != EOF )
	{
		Trace2(2, "got line \"%s\"", ExpandString(Buf, -1));

		if ( (cp = BufToks[0]) == NULLSTR )
			continue;

		BufBrks[0][0] = '\0';
		token = 1;

		if ( (cmdp = (Cmdp)bsearch(cp, (char *)Cmds, NCMDS, CMDZ, cmpcom)) == (Cmdp)0 )
		{
			last_cmd = &null_cmd;
			null_cmd.cd_name = cp;
			errp = english("unknown command");
			goto error;
		}
		else
			last_cmd = cmdp;

		aerrp = EmptyString;

		switch ( cmdp->cd_type )
		{
		case ct_breakpseudo:
		case ct_removemaps:
		case ct_licence:
			if ( BufToks[1] != NULLSTR )
			{
				BufBrks[1][0] = '\0';	/* Remove trailing tabs */
				token++;
			}
		case ct_comment:
		case ct_contact:
		case ct_edited:
		case ct_email:
		case ct_location:
		case ct_org:
		case ct_post:
		case ct_systype:
		case ct_telno:
			lp = (NLink *)0;
			ep2 = (Entry *)0;
			ep1 = (Entry *)0;
			if ( (cp = BufToks[1]) != NULLSTR )
			{
				if ( cmdp->cd_type == ct_removemaps )
					goto canon1;
				if ( cmdp->cd_type == ct_breakpseudo )
					goto canon2;
				goto comment;
			}
			goto command;
		default:
			break;	/* gcc -Wall */
		}

		if ( (cp = BufToks[1]) == NULLSTR )
		{
			errp = english("no address for");
			goto error;
		}

		BufBrks[1][0] = '\0';
		token++;

		if ( (errp = strchr(cp, LINK_C)) == NULLSTR )
		{
			if ( cmdp->cd_flags & CF_LINK )
			{
				errp = missing_link;
				goto error;
			}

			lp = (NLink *)0;
			ep2 = (Entry *)0;

			switch ( cmdp->cd_type )
			{
			case ct_address:
			case ct_add:
				if ( (ep1 = Lookup(CanonicalName(cp, buf1), RegionHash)) == (Entry *)0 )
					ep1 = EnterRegion(newstr(buf1), (bool)(cmdp->cd_type == ct_add));
				break;

			case ct_forward:
				if ( cp[0] == '*' && cp[1] == '.' )
				{
					if ( cp[2] == '\0' )
					{
						aerrp = &buf1[2];
						*aerrp = '\0';	/* WORLD */
					}
					else
						aerrp = CanonicalName(&cp[2], &buf1[2]);
					ep1 = Lookup(aerrp, FwdMapHash);
					*--aerrp = '.';
					*--aerrp = '*';
					break;
				}
canon1:				ojmp = NoJmp;
				NoJmp = true;
				if ( (aerrp = CanonicalName(cp, buf1)) == NULLSTR || aerrp == _ERROR_ )
					aerrp = strcpy(buf1, cp);
				NoJmp = ojmp;
				ep1 = Lookup(aerrp, AdrMapHash);
				break;

			case ct_map:
				aerrp = cp;
				ep1 = Lookup(cp, RegMapHash);
				break;

			case ct_ialias:
			case ct_xalias:
				aerrp = CanonicalName(cp, buf1);
				ep2 = Lookup(aerrp, AliasHash);
				ep1 = Lookup(StripCopy(buf2, aerrp), RegMapHash);
				break;

			case ct_breakall:
			case ct_caller:
			case ct_cheaproute:
			case ct_clear:
			case ct_fastroute:
			case ct_filter:
			case ct_linkname:
			case ct_noroute:
			case ct_remove:
			case ct_route:
			case ct_spooler:
			case ct_visible:
canon2:				ep1 = Lookup(CanonicalName(cp, buf1), RegionHash);
				break;

			default:
				aerrp = cp;
				ep1 = (Entry *)0;
			}
		}
		else
		{
			if ( !(cmdp->cd_flags & (CF_LINK|CF_OPTLINK)) )
			{
				errp = english("extraneous link");
				goto error;
			}

			*errp++ = '\0';

			if ( *errp == '\0' )
			{
				if ( (errp = BufToks[2]) == NULLSTR )
				{
					errp = missing_link;
					goto error;
				}

				BufBrks[2][0] = '\0';
				token++;
			}

			cp = CanonicalName(cp, buf1);
			errp = CanonicalName(errp, buf2);

			if
			(
				(ep1 = Lookup(cp, RegionHash)) != (Entry *)0
				&&
				(ep2 = Lookup(errp, RegionHash)) != (Entry *)0
			)
			{
				if ( ep1 == ep2 )
				{
					errp = english("link to self");
					goto error;
				}

				lp = IsLinked(ep1->REGION, ep2->REGION);
			}
			else
				lp = (NLink *)0;
		}

		if ( (cp = BufToks[token]) == NULLSTR )
		{
			if ( cmdp->cd_flags & CF_PARAMS )
			{
				errp = english("missing parameter list");
				goto error;
			}
		}
		else
		{
			BufBrks[token][0] = '\0';

			if ( !(cmdp->cd_flags & (CF_PARAMS|CF_OPTPARAMS)) )
			{
				errp = english("unexpected parameter list");
				goto error;
			}

			if ( BufToks[++token] != NULLSTR )
			{
				errp = english("extraneous parameters");
				goto error;
			}
comment:
			if ( *cp == QUOTE_C )
				cp++;
			else
			if ( *cp == '\0' )
				cp = NULLSTR;
		}
command:
		switch ( Pcommand(cmdp->cd_type, ep1, ep2, lp, cp, &aerrp) )
		{
		case pr_error:	errp = aerrp;	goto error;
		case pr_warn:	if ( warn )
		case pr_fwarn:		Warn(errfmt, aerrp, cmdp->cd_name, Line);
		default:	break;	/* gcc -Wall */
		}
	}

	return retval;
}



#if	DEBUG
/*
**	Decode command type.
*/

char *
cmdname(type)
	CmdType		type;
{
	register int	i;
	register Cmdp	cp;

	for ( cp = Cmds, i = NCMDS ; --i >= 0 ; cp++ )
		if ( cp->cd_type == type )
			return cp->cd_name;

	return Unknown;
}
#endif	/* DEBUG */



/*
**	Command name compare for bsearch.
*/

int
#if	ANSI_C
cmpcom(const void *cp, const void *cmdp)
#else	/* ANSI_C */
cmpcom(cp, cmdp)
	char *	cp;
	char *	cmdp;
#endif	/* ANSI_C */
{
	Trace3(5, "cmpcom \"%s\" <> \"%s\"", (char *)cp, ((Cmdp)cmdp)->cd_name);
	return strccmp((char *)cp, ((Cmdp)cmdp)->cd_name);
}



/*
**	Read a command line, ignoring comments and empty lines.
*/

static int
_getline()
{
	register char *	cp;
	register int	c;
	register char *	ep;
	register FILE *	fd	= CmdsFd;
	register bool	slosh;
	register bool	ignore;
	register bool	next_token;
	register int	token;

	NameErr		state;
	static int	last_c;

	if ( feof(fd) )
		return EOF;

	ep = BufEnd;
	state = ne_ok;

	for ( ;; )
	{
		cp = Buf;
		slosh = false;
		ignore = false;
		next_token = true;
		token = 0;

		if ( (c = last_c) != 0 && c != EOF )
		{
			last_c = 0;
			goto skipg;
		}

		while ( (c = getc(fd)) != EOF )
		{
skipg:			if ( c == '\n' )
			{
				Line++;
				if ( !slosh && (c = getc(fd)) != '\t' && c != ' ' )
					break;
			}

			if ( ignore )
				continue;

			if ( c >= 0177 || (c < 040 && c != '\t' && c != '\n') )
			{
				state = ne_illc;
				cp = Buf;
				ignore = true;
			}
			else
			if ( slosh )
			{
				slosh = false;
				*cp++ = c;
			}
			else
			if ( c == COMMENT_C )
			{
				ignore = true;
			}
			else
			if ( cp >= ep )
			{
				state = ne_toolong;
				cp = Buf;
				ignore = true;
			}
			else
			if ( c == '\\' )
			{
				slosh = true;
			}
			else
			{
				if ( c == '\t' || c == ' ' )
				{
					if ( !next_token )
					{
						if ( token < LINE_TOKENS )
							BufBrks[token] = cp;
						token++;
						next_token = true;
					}
				}
				else
				if ( next_token )
				{
					if ( token < LINE_TOKENS )
						BufToks[token] = cp;
					next_token = false;
				}

				*cp++ = c;
			}
		}

		last_c = c;

		if ( state != ne_ok )
			longjmp(NameErrJmp, (int)state);

		if ( token > 0 || !next_token )
		{
			*cp = '\0';

			if ( !next_token && token < LINE_TOKENS )
				BufBrks[token++] = cp;

			if ( token < LINE_TOKENS )
				BufToks[token] = NULLSTR;

			return 0;
		}

		if ( c == EOF )
			return EOF;
	}
}
