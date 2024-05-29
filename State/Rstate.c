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


/**	IF INPUT IS ALL BUFFERRED, NO NEED FOR Lookup() -> USE Enter()	**/

/*
**	Read a state file and structure it in memory.
*/

#define	STDIO
#define	TOKEN_DATA

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"
#include	"commands.h"

#include	<setjmp.h>


static bool	AllState;
static bool	IgnoreCRC;
static int	Port;

bool		AcceptPseudo;
bool		Foreign;
int		Last_C;
char *		RSrcAddr;
Time_t		RSrcDate;
FILE *		RStateFd;
Region *	SourceRegion;
Crc_t		StCRC;
long		TokenChar;
long		TokenCount;
int		TokenLength;

extern jmp_buf	NameErrJmp;

extern void	Map1stName _FA_((char *, Region *));
static Ulong	RsetFlags _FA_((char *));
static Token	Rtoken _FA_((Token));
#if	DEBUG >= 2
static void	TokenTrace _FA_((int, Token, Tokens));
#endif	/* DEBUG >= 2 */



void
Rstate(fd, foreign, sourceaddr, sourcedate, ignoreCRC)
	FILE *			fd;
	bool			foreign;
	char *			sourceaddr;
	Time_t			sourcedate;
	bool			ignoreCRC;
{
	register Region *	rp1;
	register Region *	rp2;
	register Entry *	ep1;
	register Entry *	ep2;
	register NLink *	lp;
	register char *		buf;
	register Token		token;
	register CmdType	ctype;
	static Token		last_token;
	register char *		errs1;
	char *			errs2;
	bool			dir_in;
	static char *		statefiletype;
	NLink			null_link;
	char			mapbuf[TOKEN_SIZE+1];

	RStateFd = fd;
	IsCommand = false;
	AllState = false;
	AcceptPseudo = false;

	rp1 = rp2 = (Region *)0;	/* gcc -Wall */
	ep1 = ep2 = (Entry *)0;		/* gcc -Wall */
	ctype = (CmdType)0;		/* gcc -Wall */

	if ( (Foreign = foreign) )
	{
		Port = 1;
		statefiletype = english("foreign");
	}
	else
	{
		Port = 0;
		statefiletype = english("local");
	}

	RSrcAddr = sourceaddr;
	RSrcDate = sourcedate;
	SourceRegion = (Region *)0;
	IgnoreCRC = ignoreCRC;

	Trace6(
		1,
		"Rstate(%d,%s,%s,%lu,%s)",
		fileno(fd),
		statefiletype,
		RSrcAddr==NULLSTR?EmptyString:RSrcAddr,
		(PUlong)RSrcDate,
		IgnoreCRC?"ignoreCRC":EmptyString
	);

	StCRC = 0;
	Last_C = EOF;
	token = t_eof;
	last_token = t_eof;
	TokenChar = 0;
	TokenCount = 0;

	switch ( (NameErr)setjmp(NameErrJmp) )
	{
	case ne_ok:		break;
	default:		Fatal1(english("unrecognised NameErr"));
	case ne_floor:		errs2 = Ne_floor;	goto error;
	case ne_illc:		errs2 = Ne_illc;	goto error;
	case ne_noname:		errs2 = Ne_noname;	goto error;
	case ne_notype:		errs2 = Ne_notype;	goto error;
	case ne_null:		errs2 = Ne_null;	goto error;
	case ne_toolong:	errs2 = Ne_toolong;	goto error;
	case ne_unktype:	errs2 = Ne_unktype;	goto error;
	case ne_illtype:	errs2 = Ne_illtype;	goto error;

error:		errs1 = english("%s \"%s\" at byte %ld in %s statefile");

		if ( !Foreign && ignoreCRC )
		{
			Warn(errs1, errs2, ExpandString(&Buf[1], TokenLength), TokenChar, statefiletype);
			token = last_token;	/* Ensure all registers are re-initialised */
		}
		else
		{
			Error(errs1, errs2, ExpandString(&Buf[1], TokenLength), TokenChar, statefiletype);
			AnteState = true;
			return;
		}
	}

	errs2 = EmptyString;
	ep2 = (Entry *)0;
	buf = &Buf[1];
	lp = &null_link;

	for ( ;; )
	{
		dir_in = false;

		switch ( token = Rtoken(token) )
		{
		case t_address:
			if ( (ep1 = Lookup(buf, RegionHash)) == (Entry *)0 )
				ep1 = EnterRegion(CanonicalName(buf, NULLSTR), Foreign);
			rp1 = ep1->REGION;
			last_token = token;
			if ( !Foreign )
			{
				ctype = ct_address;
				goto docommand;
			}
			if ( !RcheckAddress(rp1) )
			{
				AnteState = true;
				return;
			}
			if ( rp1->rg_flags & RF_NEW )
				rp1->rg_visible = CommonRegion(rp1, HomeRegion);
			if ( NOADDRCOMPL && rp1->rg_up == HomeRegion->rg_up )
				Map1stName(rp1->rg_name, rp1);
			if ( InRegion(VisRegion, rp1->rg_visible) )
				AcceptPseudo = true;
			continue;

		case t_caller:
			ctype = ct_caller;
docommand:		if ( Pcommand(ctype, ep1, ep2, lp, (buf[0]=='\0')?NULLSTR:buf, &errs2) == pr_error )
				goto error;
			continue;

		case t_cheaproute:
			ctype = ct_cheaproute;
			if ( (ep1 = Lookup(buf, RegionHash)) == (Entry *)0 )
				ep1 = Lookup(CanonicalName(buf, mapbuf), RegionHash);
			goto map;

		case t_comment:
			if ( Foreign )
			{
				SourceComment = newstr(buf);
				continue;
			}
			HomeComment = newstr(buf);
			SplitComment();
			continue;

		case t_cost:
			ctype = ct_cost;
			break;		/* dir_in = false; */

		case t_date:
			if ( (sourcedate = DecodeNum(buf, TokenLength)) > rp1->rg_state )
				rp1->rg_state = sourcedate;
			continue;

		case t_delay:
			ctype = ct_delay;
			dir_in = true;
			break;

		case t_eof:
			goto out;

		case t_error:
			errs2 = english("illegal statefile token");
			goto error;

		case t_expect:
			errs2 = english("expected field missing");
			goto error;

		case t_filter:
			ctype = ct_filter;
			goto docommand;

		case t_fastroute:
			ctype = ct_fastroute;
			if ( (ep1 = Lookup(buf, RegionHash)) == (Entry *)0 )
				ep1 = Lookup(CanonicalName(buf, mapbuf), RegionHash);
			goto map;

		case t_forward:
			ctype = ct_forward;
			errs1 = strcpyend(mapbuf, "*.");
			(void)strcpy(errs1, buf);
			ep1 = Lookup(buf, FwdMapHash);
			errs2 = mapbuf;
			continue;	/* Expect t_mapval */

		case t_ialias:
			ctype = ct_ialias;
			ep2 = Lookup(buf, AliasHash);
			ep1 = Lookup(StripCopy(mapbuf, buf), RegMapHash);
map:			errs2 = strcpy(mapbuf, buf);
			continue;	/* Expect t_mapval */

		case t_lflags:
			if
			(
				!Foreign
				||
				rp1 == SourceRegion
				||
				rp1->rg_state == 0
				||
				(lp->nl_flags & FL_DUPROUTE)
			)
			{
				register Ulong	ul;

				DODEBUG(if ( (ul = Traceflag) >= 3 )
				{
					Traceflag = 0;
					(void)fprintf(TraceFd, "\t  nl_flags [%lx] ", (PUlong)lp->nl_flags);
					PrintFlags(TraceFd, lp->nl_flags);
					(void)putc('\n', TraceFd);
					Traceflag = ul;
				});

				if ( Foreign )
					ul = FL_NOIMPORT;
				else
					ul = FL_INTERNAL;

				lp->nl_flags &= ul;
				lp->nl_flags |= RsetFlags(buf) & ~ul;

				DODEBUG(if ( (ul = Traceflag) >= 3 )
				{
					Traceflag = 0;
					(void)fprintf(TraceFd, "\t  nl_flags [%lx] ", (PUlong)lp->nl_flags);
					PrintFlags(TraceFd, lp->nl_flags);
					(void)putc('\n', TraceFd);
					Traceflag = ul;
				});
			}
			continue;

		case t_link:
			last_token = t_nextregion;
			if ( (ep2 = Lookup(buf, RegionHash)) == (Entry *)0 )
				ep2 = EnterRegion(CanonicalName(buf, NULLSTR), Foreign);
			rp2 = ep2->REGION;
			lp = IsLinked(rp1, rp2);
			if
			(
				Foreign
				&&
				(
					(rp1 == HomeRegion && lp == (NLink *)0)
					||
					(rp2 == HomeRegion && lp == (NLink *)0)
					||
					(
						rp1 != SourceRegion
						&&
						rp2 != SourceRegion
						&&
						rp1->rg_state != 0
					)
				)
			)
			{
				lp = &null_link;
				token = t_nextlink;
				continue;
			}
			Trace2(2, " -> \"%s\"", rp2->rg_name);
			if ( lp != (NLink *)0 )
			{
				if ( !(lp->nl_flags & FL_PSEUDO) )
				{
					lp->nl_flags &= ~FL_REMOVE;
					continue;
				}
				UnLink(rp1, rp2);
				lp = (NLink *)0;
			}
			if ( Pcommand(ct_halflink, ep1, ep2, lp, NULLSTR, &errs2) == pr_error )
				goto error;
			if ( (lp = IsLinked(rp1, rp2)) == (NLink *)0 )
			{
				errs2 = english("could not make link");
				goto error;
			}
			if ( Foreign )
				lp->nl_flags |= FL_DUPROUTE;	/* Flag new link added by foreign host */
			continue;

		case t_linkname:
			ctype = ct_linkname;
			goto docommand;

		case t_mapadr:
			ctype = ct_forward;
			ep1 = Lookup(buf, AdrMapHash);
			goto map;

		case t_mapreg:
			ctype = ct_map;
			ep1 = Lookup(buf, RegMapHash);
			goto map;

		case t_maptyp:
			ctype = ct_maptype;
			ep1 = Lookup(buf, TypMapHash);
			goto map;

		case t_mapval:
			last_token = token;
			goto docommand;

	/*	case t_oldaddress:	(see above t_xalias)	*/

		case t_ordertypes:
			if ( Foreign )
				AllState = true;
			else
			if ( (errs2 = ParseOrderTypes(buf)) != NULLSTR && !ignoreCRC )
				goto error;
			continue;

		case t_region:
			last_token = t_nextregion;
			if ( (ep1 = Lookup(buf, RegionHash)) == (Entry *)0 )
				ep1 = EnterRegion(CanonicalName(buf, NULLSTR), Foreign);
			rp2 = rp1 = ep1->REGION;
			Trace2(2, "REGION \"%s\"", rp1->rg_name);
			if ( rp1->rg_flags & RF_NEW )
				rp1->rg_visible = CommonRegion(rp1, HomeRegion);
			if ( NOADDRCOMPL && rp1->rg_up == HomeRegion->rg_up )
				Map1stName(rp1->rg_name, rp1);
			if
			(
				!Foreign
				||
				!AcceptPseudo
				||
				(
					!(rp1->rg_flags & (RF_NEW|RF_PSEUDO))
					&&
					(
						!(rp1->rg_flags & RF_HOME)
						||
						InRegion(rp1, HomeRegion->rg_visible)
					)
				)
			)
				continue;
			if ( (lp = IsLinked(SourceRegion, rp1)) == (NLink *)0 )
				(lp = MakeLink(SourceRegion, rp1))->nl_flags |= FL_PSEUDO;
			else
			if ( lp->nl_flags & FL_PSEUDO )
				lp->nl_flags &= ~FL_REMOVE;
			continue;

		case t_restrict:
			ctype = ct_restrict;
			dir_in = true;
			break;

		case t_rflags:
			if ( !Foreign || rp1 == SourceRegion || rp1->rg_state == 0 )
			{
				register Ulong	ul;

				if ( Foreign )
					ul = RF_NOIMPORT;
				else
					ul = RF_INTERNAL;

				rp1->rg_flags &= ul;
				rp1->rg_flags |= RsetFlags(buf) & ~ul;
			}
			continue;

		case t_speed:
/*			ctype = ct_speed;
**			break;		*//* dir_in = false; *//*
*/			continue;

		case t_spooler:
			ctype = ct_spooler;
			goto docommand;

		case t_traffic:
/*			ctype = ct_traffic;
**			dir_in = true;
**			break;
*/			continue;

		case t_visible:
			ctype = ct_visible;
			if ( !Foreign || rp1 == SourceRegion || rp1->rg_state == 0 )
				goto docommand;
			continue;

		case t_oldaddress:
			ep1 = Lookup(buf, RegionHash);
			if ( !Foreign )
			{
				if ( ep1 == (Entry *)0 )
					OldAddress = newstr(buf);
				continue;
			}
			if
			(
				ep1 != (Entry *)0
#if	0
				&&
				(
					(rp1 = ep1->REGION)->rg_down != (Region *)0
					||
					!RemoveRegion(rp1)	/* Security hole? */
				)
#endif	/* 0 */
			)
			{
				Warn(english("foreign site \"%s\" claiming old address \"%s\""), RSrcAddr, buf);
				continue;
			}
		case t_xalias:
			if ( Foreign )
			{
				ctype = ct_ialias;
				errs1 = RSrcAddr;
			}
			else
			{
				ctype = ct_xalias;
				errs1 = NULLSTR;
			}
			errs2 = buf;
			ep1 = Lookup(StripCopy(mapbuf, buf), RegMapHash);
			ep2 = Lookup(buf, AliasHash);
			(void)Pcommand(ctype, ep1, ep2, (NLink *)0, errs1, &errs2);
			continue;

		default:
			if ( Foreign )
			{
				Report3(english("Unknown foreign statefile token [%d] ignored: \"%s\""), (int)token, buf);
				token = last_token;
				continue;		/* Allow for new tokens */
			}
			Fatal3(english("Rstate: unknown token [%d] value \"%s\""), (int)token, buf);
		}

		/*
		**	The following changes are legal for SourceRegion:
		**	restrictions IN, delays IN, costs OUT, flags OUT;
		**	others are legal if no state received from owner.
		*/

		if ( !Foreign || (lp->nl_flags & FL_DUPROUTE) )
			goto docommand;

		if ( dir_in )
		{
			if ( rp2 == SourceRegion || rp2->rg_state == 0 )
				goto docommand;
		}
		else
			if ( rp1 == SourceRegion || rp1->rg_state == 0 )
				goto docommand;
	}

out:
	if
	(
		!ignoreCRC
		&&
		TokenCount > 0
		&&
		(
			getc(fd) != LOCRC(StCRC)
			||
			getc(fd) != HICRC(StCRC)
		)
	)
	{
		Error(english("bad %s statefile CRC"), statefiletype);
		AnteState = true;
		return;
	}

	if ( Foreign )
	{
		if ( SourceRegion == (Region *)0 )
		{
			Error(english("null %s statefile"), statefiletype);
			AnteState = true;
			return;
		}

		if ( RSrcDate > 0 )
		{
			SourceRegion->rg_state = RSrcDate;
			SourceRegion->rg_flags &= ~RF_FOREIGN;
		}

#		if	SUN3 == 1
		ep1 = (Entry *)0;
		ep2 = Lookup(RSrcAddr, AliasHash);
		errs2 = RSrcAddr;
		(void)Pcommand(ct_ialias, ep1, ep2, (NLink *)0, RSrcAddr, &errs2);
#		endif	/* SUN3 == 1 */
	}
}



/*
**	Read a separator followed by a token (name or number)
**	and return token type.
*/

static Token
Rtoken(last_token)
	Token		last_token;
{
	register char *	s = Buf;
	register int	c;
	register FILE *	fd = RStateFd;
	register char *	ep = BufEnd;
	register Token	token = t_null;
	register Tokens	accept;

	DODEBUG(if((int)last_token>=(int)t_max)Fatal(english("Rtoken(%d): token range"),last_token));

	accept = Accept[(int)last_token][Port];

	DODEBUG(if(accept==(Tokens)0)Fatal(english("Rtoken(%d): accept==0"),last_token));
	DODEBUG(if(Traceflag>=5){TokenTrace(5,last_token,accept);});

	if ( (c = Last_C) != EOF )
	{
		Last_C = EOF;
		goto next;
	}

	for ( ;; )
	{
		if ( (c = getc(fd)) == EOF )
		{
			Trace1(1, "Rtoken at EOF!");
			if ( token == t_null )
				return t_eof;
			goto eof;
		}

next:		*s++ = (char)c;

		if ( c > 0177 )
		{
			if ( (c &= 0177) > 037 )
			{
				TokenLength = (s - Buf) - 1;
				TokenChar += s - Buf;
				return t_error;
			}

			if ( token == t_null )
			{
				if ( !(accept & (1L<<c)) )
				{
					if ( accept & T_EXPECT )
					{
						token = t_expect;
						goto backout;
					}
					if ( c == C_EOF )
					{
						token = (Token)c;
						goto backout;
					}
					token = t_null;
				}
				else
					token = (Token)c;

				if ( s != &Buf[1] )
				{
					c = *--s;
					if ( !IgnoreCRC )
						StCRC = acrc(StCRC, Buf, s - Buf);
					TokenChar += s - Buf;
					Trace4(
						3,
						"Ignore token %ld \"%s\": \"%s\"",
						TokenCount+1,
						TokenNames[Buf[0]&037],
						ExpandString(Buf, s-Buf)
					);
					Buf[0] = c;
					s = &Buf[1];
				}

				TokenCount++;
				continue;
			}
			else
			{
backout:			Last_C = *(Uchar *)--s;
eof:				if ( (c = s - Buf) > 0 )
				{
					if ( !IgnoreCRC )
						StCRC = acrc(StCRC, Buf, c);
					TokenChar += c;
					TokenLength = --c;
				}
				else
					TokenLength = 0;
				*s = '\0';
				Trace4(
					4,
					"Rtoken %ld \"%s\": \"%s\"",
					TokenCount,
					TokenNames[(int)token],
					ExpandString(Buf, TokenLength+1)
				);
				return token;
			}
		}

		if ( s > ep )
		{
			TokenLength = (s - Buf) - 1;
			TokenChar += s - Buf;
			longjmp(NameErrJmp, (int)ne_toolong);
		}
	}
}



/*
**	Decode state flags from string.
*/

static Ulong
RsetFlags(cp)
	register char *	cp;
{
	register int	c;
	register Ulong	flags = 0;

	Trace2(3, "RsetFlags(%s)", cp);

	while ( (c = *cp++) )
		flags |= 1 << (c-'A');

	return flags;
}



/*
**	Check foreign address.
*/

bool
RcheckAddress(rp)
	register Region *	rp;
{
	Entry *			sep;

	Trace2(1, "RcheckAddress(%s)", rp->rg_name);

	if ( (sep = Lookup(RSrcAddr, RegionHash)) == (Entry *)0 )
		SourceRegion = (Region *)0;
	else
	if ( (SourceRegion = sep->REGION) == HomeRegion )
	{
		Error
		(
			english("foreign source address \"%s\" same as Home"),
			RSrcAddr
		);
		return false;
	}

	if ( SourceRegion != rp )
	{
#		if	SUN3 == 1
		char *	sp1 = StripTypes(RSrcAddr);
		char *	sp2 = StripTypes(rp->rg_name);

		if ( strcmp(sp1, sp2) != STREQUAL )
#		endif	/* SUN3 == 1 */
		{
			Error
			(
				english("foreign source address \"%s\" != one in statefile \"%s\""),
				RSrcAddr,
				rp->rg_name
			);
			return false;
		}

#		if	SUN3 == 1
		SourceRegion = rp;
		RSrcAddr = rp->rg_name;
#		endif	/* SUN3 == 1 */
	}

	if ( RSrcDate > 0 )
	{
		extern Time_t	Time;
		char		buf1[DATETIMESTRLEN];
		char		buf2[DATETIMESTRLEN];

		if ( rp->rg_state >= RSrcDate && rp->rg_state < Time )
		{
			Warn
			(
				english("state data from \"%s\" [%s] older than %s"),
				rp->rg_name,
				DateTimeStr(RSrcDate, buf1),
				DateTimeStr(rp->rg_state, buf2)
			);
			return false;
		}
	}

	/*
	**	Clear variables which this statefile may legally set.
	*/

	{
		register NLink *	lp;
		register NLink *	olp;

		for ( lp = rp->rg_links ; lp != (NLink *)0 ; lp = lp->nl_next )
			if ( lp->nl_to != HomeRegion && (AllState || !(lp->nl_flags & FL_PSEUDO)) )
			{
				/*
				**	Don't break these links,
				**	as we don't want to lose data such as
				**	delay/cost/restrict.
				*/

				lp->nl_flags |= FL_REMOVE;		/* Out */

				if ( (olp = IsLinked(lp->nl_to, rp)) != (NLink *)0 )
					olp->nl_flags |= FL_REMOVE;	/* In */
			}
	}

	RemEntMap(RSrcAddr, AliasHash);

	return true;
}



#if	DEBUG >= 2
static void
TokenTrace(level, token, accept)
	int		level;
	Token		token;
	Tokens		accept;
{
	register char *	cp;
	register int	i;
	register Tokens	t;
	static char *	buf;

	if ( buf == NULLSTR )
	{
		int	len = 0;

		for ( i = 0 ; i < (int)t_n ; i++ )
			len += strlen(TokenNames[i]);

		buf = Malloc(len+i);
	}

	cp = buf;
	*cp++ = ' ';

	for ( t = 1, i = 0 ; i < (int)t_n ; i++, t <<= 1 )
		if ( accept & t )
		{
			cp = strcpyend(cp, TokenNames[i]);
			*cp++ = ',';
		}

	*--cp = '\0';

	Trace(level, "Rtoken(%s) <=%s", TokenNames[(int)token], buf);
}
#endif	/* DEBUG >= 2 */
