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
**	Read a SUN III state file and structure it in memory.
*/

#define	STDIO
#define	TOKEN_3_DATA

#include	"global.h"

#if	SUN3 == 1

#include	"address.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"
#include	"sun3state.h"
#include	"commands.h"

#include	<setjmp.h>


/*
**	Flag results.
*/

enum Node_flags
{
	msg_link, local_node, terminal_node
};

typedef Ushort		NFlags;

#define	LOCAL_NODE	(NFlags)(1L<<(int)local_node)
#define	MSG_LINK	(NFlags)(1L<<(int)msg_link)
#define	TERMINAL_NODE	(NFlags)(1L<<(int)terminal_node)

/*
**	Build details of each link,
**	one of these for each entry in ``linkhash''.
*/

typedef struct
{
	char *	name;		/* complete name */
	char *	hier;		/* hierarchy part */
	char *	vis;		/* visible part */
	int	length;		/* of name */
	Flags	lflags;		/* link flags */
	NFlags	nflags;		/* node flags */
	Ulong	cost;		/* link cost */
	Ulong	delay;		/* link delay */
}
			LinkNode;

#define	NODE(P)		((LinkNode *)((P)->REGION))
#define	SET_NODE(P,V)	(P)->REGION = (Region *)(V)

static bool		IgnoreCRC;
static char		Oz[]	= "oz";
static Entry *		SourceEntry;
static LinkNode		SrcNode;		/* Source details */
static int		State;			/* Read state */
static char *		Xalias;

/*
**	Miscellaneous variables imported from ``Rstate()''.
*/

extern char		_ERROR_[];
extern bool		Foreign;
extern int		Last_C;
extern jmp_buf		NameErrJmp;
extern bool		NoJmp;
extern char *		RSrcAddr;
extern Time_t		RSrcDate;
extern FILE *		RStateFd;
extern Region *		SourceRegion;
extern Crc_t		StCRC;
extern long		TokenChar;
extern long		TokenCount;
extern int		TokenLength;

/*
**	Local routines.
*/

static void		Conv34Address _FA_((char *, char *));
static PrType		EnterNode _FA_((LinkNode *, char *, char **));
static void		R3setFlags _FA_((LinkNode *, char *));
static T3token		R3token _FA_((T3token));
static void		SetLinkNames _FA_((LinkNode *, char *, char *));
#if	DEBUG >= 2
static void		Token3Trace _FA_((int, T3token, T3tokens));
#endif	/* DEBUG >= 2 */



void
R3state(fd, foreign, sourceaddr, sourcedate, ignoreCRC)
	FILE *			fd;
	bool			foreign;
	char *			sourceaddr;
	Time_t			sourcedate;
	bool			ignoreCRC;
{
	register LinkNode *	np;
	register Entry *	ep;
	register char *		buf = &Buf[1];
	register T3token	token;
	register char *		namebufp;
	register char *		errs1;
	register LinkNode *	lnp;
	static T3token		last_token;
	char *			errs2;
	bool			first_node;
	bool			link_name;		/* Some links won't have hierarchies */
	bool			hier_name;		/* Some nodes won't have hierarchies */
	char			namebuf[TOKEN_SIZE+1];	/* NB: NOT TOKEN_3_SIZE because of CanonicalName() */
	Entry *			linkhash[HASH_SIZE];	/* Names of links to source only */

	RStateFd = fd;
	IsCommand = false;

	if ( sourceaddr != NULLSTR )
	{
		first_node = NoJmp;	/* Remember setting */
		NoJmp = true;
		Conv34Address(sourceaddr, buf);
		if ( (RSrcAddr = CanonicalName(buf, NULLSTR)) == NULLSTR || RSrcAddr == _ERROR_ )
			RSrcAddr = EmptyString;
		NoJmp = first_node;
	}
	DODEBUG(else RSrcAddr = EmptyString);

	Foreign = foreign;
	RSrcDate = sourcedate;
	SourceRegion = (Region *)0;
	IgnoreCRC = ignoreCRC;

	Trace6(
		1,
		"R3state(%d, %s, %s, %lu, %s)",
		fileno(fd),
		foreign?"foreign":"local",
		RSrcAddr,
		(PUlong)RSrcDate,
		ignoreCRC?"ignoreCRC":"checkCRC"
	);

	StCRC = 0;
	Last_C = EOF;
	last_token = token = t3_start;
	TokenChar = 0;
	TokenCount = 0;
	np = lnp = (LinkNode *)0;
	namebufp = namebuf;
	link_name = false;
	hier_name = true;
	first_node = true;
	State = 0;
	(void)strclr((char *)linkhash, sizeof linkhash);

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

error:		errs1 = english("%s \"%s\" for token %ld/%ld in foreign SUN III statefile");
		Error(errs1, errs2, ExpandString(buf, TokenLength), TokenCount, TokenChar);
		AnteState = true;
		return;
	}

	for ( ;; )
	{
		/*
		**	Tokens are in first encountered statefile order.
		*/

		errs2 = EmptyString;

		switch ( token = R3token(token) )
		{
		case t3_domhier:
			hier_name = false;
node:			*namebufp++ = DOMAIN_SEP;	/* Build source hierarchy */
			namebufp = strcpyend(namebufp, buf);
			if ( hier_name )
			{
				hier_name = false;
				token = t3_domhier;
			}
			last_token = token;
			continue;

		case t3_node:
			last_token = token;
			if ( first_node )	/* First node is source's */
			{
				first_node = false;
				(void)strcpy(&buf[TokenLength], namebuf);	/* Saved source hierarchy  */
				lnp = np = &SrcNode;
				SetLinkNames(np, buf, namebuf);
				buf[TokenLength] = '\0';
			}
			else
			{
				if ( (np = lnp) != (LinkNode *)0 )
					switch ( EnterNode(np, namebuf, &errs2) )
					{
					case pr_exit:	return;
					case pr_error:	goto error;
					default:	break;	/* gcc -Wall */
					}
				State = 1;	/* Only node name, domain, flags */
				if ( (ep = Lookup(buf, linkhash)) == (Entry *)0 )
				{
					lnp = (LinkNode *)0;
					token = t3_nextnode;	/* No link, try again */
					continue;
				}
				lnp = np = NODE(ep);
			}
			continue;

		case t3_domain:			/* First is primary domain of node == visible region */
			namebuf[0] = TYPE_SEP;
			namebufp = strcpyend(&namebuf[1], buf);
			*namebufp++ = DOMAIN_SEP;
			*namebufp = '\0';
			while ( (namebufp = StringMatch(np->hier, namebuf)) == NULLSTR )
			{
				/** No match -- insert domain in hierarchy **/

				DODEBUG(if(np->vis!=NULLSTR)Fatal("Hierarchy"));

				np->vis = newstr(namebuf);			/* Save for next match */
				namebufp = np->hier;
				*--namebufp = '\0';
				namebufp = strcpyend(buf, &np->name[2]);	/* Node name (skip type) */
				*namebufp++ = DOMAIN_SEP;
				namebufp = strcpyend(namebufp, &namebuf[1]);	/* Primary domain and DOMAIN_SEP */
				np->name[np->length] = '\0';
				(void)StripCopy(namebufp, np->hier);		/* Old hierarchy */
				SetLinkNames(np, buf, namebuf);
				(void)strcpy(namebuf, np->vis);			/* Put back for match */
			}
			np->vis = --namebufp;		/* Back over type char */
			continue;			/* Other domains skipped */

		case t3_nflags:
			R3setFlags(np, buf);
			continue;

		case t3_comment:
			buf[TokenLength++] = '\n';
			buf[TokenLength] = '\0';
			if ( foreign )
			{
				SourceComment = newstr(buf);
				continue;
			}
			HomeComment = concat(SiteInfoNames[0].si_name, StInfoSep, buf, NULLSTR);
			SplitComment();
			continue;

		case t3_link:
			ep = Enter(newstr(buf), linkhash);	/* Enter a link name */
			np = Talloc0(LinkNode);			/* Create structure for details */
			SET_NODE(ep, np);			/* And tie it in */
			namebufp = strcpyend(namebuf, buf);	/* Save link name & wait for nodehier */
			link_name = true;			/* If no hier, substitute Oz */
			continue;

		case t3_nodehier:
node_hier:		last_token = token = t3_nodehier;
			link_name = false;
			*namebufp++ = DOMAIN_SEP;
			(void)strcpy(namebufp, buf);
			SetLinkNames(np, namebuf, buf);		/* Set name and hier */
			continue;

		case t3_cost:
			np->cost = (Ulong)atol(buf);
			continue;

		case t3_speed:
			/** Could maybe use table lookup to convert speed to delay factor?? **/
/*			np->speed = (Ulong)atol(buf);
*/			continue;

		case t3_lflags:
			R3setFlags(np, buf);
			continue;

		case t3_xalias:
			Xalias = concat("9=", buf, NULLSTR);	/* Remember first only (others ignored) */
			continue;

		case t3_eof:
			goto out;

		case t3_error:
			errs2 = english("illegal statefile token");
			goto error;

		case t3_expect:
			if ( link_name || hier_name )
			{
				(void)strcpy(buf, Oz);
				if ( link_name )
					goto node_hier;
				goto node;
			}
			errs2 = english("expected field missing");
			goto error;

		default:
			Report3(english("Unknown SUN III statefile record [%d] ignored: \"%s\""), token, buf);
			token = last_token;
			continue;	/* Allow for new tokens */
		}
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
		Error(english("bad SUN III statefile CRC"));
		AnteState = true;
		return;
	}

	if ( SourceRegion == (Region *)0 )
	{
		Error(english("null SUN III statefile"));
		AnteState = true;
		return;
	}

	if ( RSrcDate > 0 )
	{
		SourceRegion->rg_state = RSrcDate;
		SourceRegion->rg_flags |= RF_FOREIGN;
	}
}



/*
**	Convert (partially converted) SUN III address to Sun4 internal format.
**
**	Add internal types (from Types.c) and append country code to any Oz.
*/

static void
Conv34Address(from, to)
	char *		from;
	register char *	to;
{
	register char *	cp;
	register char *	np;
	register int	count;

	Trace2(2, "Conv34Address(%s)", from);

	/*
	**	Count domains, and add country code for any Oz.
	*/

	for ( count = 2, cp = from ;; )
	{
		if ( (np = strchr(cp, '.')) == NULLSTR )
		{
			if ( strcmp(cp, Oz) == STREQUAL )
			{
				(void)strcat(cp, OZCC);
				count++;
			}
			break;
		}
		count++;
		cp = ++np;
	}

	/*
	**	Copy over each domain, adding type if necessary.
	*/

	for ( cp = from ;; )
	{
		if ( (np = strchr(cp, '.')) != NULLSTR )
			*np = '\0';

		if ( --count == 1 )
			count = 0;	/* Skip ADMD */

		if ( cp[1] != TYPE_SEP )
		{
			if ( cp == from )
				*to++ = '9';
			else
				*to++ = '0' + count;
			*to++ = TYPE_SEP;
		}

		to = strcpyend(to, cp);

		if ( (cp = np) == NULLSTR )
			break;

		*cp++ = '.';
		*to++ = DOMAIN_SEP;
	}
}



/*
**	Enter accumulated node/link details.
*/

static PrType
EnterNode(np, buf, errp)
	register LinkNode *	np;
	register char *		buf;	/* Work area */
	register char **	errp;
{
	register Entry *	ep;
	register Region *	rp;
	register NLink *	lp;
	register NLink *	olp;
	register bool		cmd_ok;

	np->name[np->length] = '\0';	/* Remove trailing DOMAIN_SEP */

	Trace2(2, State?" -> \"%s\"":"SOURCE REGION \"%s\"", np->name);

	if ( State && !(np->nflags & MSG_LINK) )
		return pr_ok;

	if ( (ep = Lookup(CanonicalName(np->name, buf), RegionHash)) == (Entry *)0 )
		ep = EnterRegion(newstr(buf), Foreign);

	rp = ep->REGION;

/*	if ( (np->nflags & LOCAL_NODE) && !(rp->rg_up->rg_flags & RF_HOME) && Foreign )
**	{
**		Report("\"%s\" is marked LOCAL", rp->rg_name);
**		if ( State == 0 )
**		{
**			AnteState = true;
**			return pr_exit;
**		}
**		return pr_ok;
**	}
*/
	if ( !Foreign || State == 0 || rp->rg_state == 0 )
		cmd_ok = true;
	else
		cmd_ok = false;

	if ( np->vis != NULLSTR )
	{
		if ( cmd_ok && Pcommand(ct_visible, ep, (Entry *)0, (NLink *)0, np->vis, errp) == pr_error )
			return pr_error;
	}
	else
	if ( rp->rg_flags & RF_NEW )
		rp->rg_visible = CommonRegion(rp, State?SourceRegion:HomeRegion);

	if ( State == 0 )
	{
		SourceEntry = ep;

		if ( !Foreign )
		{
			rp->rg_flags |= RF_FOREIGN;
			SourceRegion = rp;
			return Pcommand(ct_address, ep, (Entry *)0, (NLink *)0, buf, errp);
		}

		RSrcAddr = rp->rg_name;	/* Ignore common SUN III addressing bug */

		if
		(
			(!(rp->rg_flags & RF_FOREIGN) && rp->rg_state != 0)
			||
			!RcheckAddress(rp)
		)
		{
			AnteState = true;
			return pr_exit;
		}

		if ( rp->rg_up != VisRegion )
		{
			/*
			**	Sun3 nodes expect general visibility, unfortunately.
			*/

			*errp = FirstDomain(buf);

			if ( Pcommand(ct_ialias, Lookup(*errp, RegMapHash), Lookup(*errp, AliasHash), (NLink *)0, np->name, errp) == pr_error )
				return pr_error;

			/*
			**	If "ialias" failed ('cos visible region too low),
			**	"map" in the node name anyway, sigh.
			*/

			*errp = FirstName(buf);	/* Strip type */

			if
			(
				Lookup(*errp, RegMapHash) == (Entry *)0	/* "ialias" failed */
				&&
				Lookup(*errp, DomMapHash) == (Entry *)0	/* Don't override anything */
				&&
				Pcommand(ct_map, (Entry *)0, (Entry *)0, (NLink *)0, np->name, errp) == pr_error
			)
				return pr_error;
		}

		if ( (*errp = Xalias) == NULLSTR )
			return pr_ok;

		return Pcommand(ct_ialias, Lookup(Xalias, RegMapHash), Lookup(Xalias, AliasHash), (NLink *)0, np->name, errp);
	}

	if ( Foreign && rp == HomeRegion )
	{
		if
		(
			(lp = IsLinked(SourceRegion, rp)) == (NLink *)0
			||
			(olp = IsLinked(rp, SourceRegion)) == (NLink *)0
		)
			return pr_ok;

		goto params;
	}

	if ( cmd_ok && rp->rg_up != VisRegion && !(np->nflags & LOCAL_NODE) )
	{
		/*
		**	Sun3 nodes expect general visibility, unfortunately.
		*/

		*errp = FirstDomain(buf);

		if ( Pcommand(ct_ialias, Lookup(*errp, RegMapHash), Lookup(*errp, AliasHash), (NLink *)0, np->name, errp) == pr_error )
			return pr_error;

		/*
		**	If "ialias" failed ('cos visible region too low),
		**	"map" in the node name anyway, sigh.
		*/

		*errp = FirstName(buf);	/* Strip type */

		if
		(
			Lookup(*errp, RegMapHash) == (Entry *)0	/* "ialias" failed */
			&&
			Lookup(*errp, DomMapHash) == (Entry *)0	/* Don't override anything */
			&&
			Pcommand(ct_map, (Entry *)0, (Entry *)0, (NLink *)0, np->name, errp) == pr_error
		)
			return pr_error;
	}

	if ( (lp = IsLinked(SourceRegion, rp)) != (NLink *)0 )
	{
		if ( lp->nl_flags & FL_PSEUDO )
		{
			UnLink(SourceRegion, rp);
			lp = (NLink *)0;
		}
		else
			lp->nl_flags &= ~FL_REMOVE;
	}

	if ( lp == (NLink *)0 )
	{
		if ( Pcommand(ct_link, SourceEntry, ep, lp, NULLSTR, errp) == pr_error )
			return pr_error;

		lp = IsLinked(SourceRegion, rp);
	}

	if ( (olp = IsLinked(rp, SourceRegion)) != (NLink *)0 )
	{
		if ( olp->nl_flags & FL_PSEUDO )
		{
			UnLink(rp, SourceRegion);
			olp = (NLink *)0;
		}
		else
			olp->nl_flags &= ~FL_REMOVE;
	}

	if ( olp == (NLink *)0 )
	{
		if ( Pcommand(ct_halflink, ep, SourceEntry, olp, NULLSTR, errp) == pr_error )
			return pr_error;

		olp = IsLinked(rp, SourceRegion);
	}

params:
	np->lflags &= ~FL_NOIMPORT;
	np->lflags |= (SrcNode.lflags & FL_FOREIGN);

	lp->nl_flags &= FL_NOIMPORT;
	lp->nl_flags |= np->lflags;

	np->lflags &= ~FL_FILTERED;	/* Only outward */

	olp->nl_flags &= FL_NOIMPORT;
	olp->nl_flags |= np->lflags;

	/*
	**	Restricting ``terminal'' nodes to node-level, while correct,
	**	leads to gateway problems, so use "hier" instead.
	*/

	if ( SrcNode.nflags & (TERMINAL_NODE|LOCAL_NODE) )
	{
		if ( Pcommand(ct_restrict, ep, SourceEntry, olp, SrcNode./*name*/hier, errp) == pr_error )
			return pr_error;
	}
/*	else
**	if ( SrcNode.nflags & LOCAL_NODE )
**	{
**		if ( Pcommand(ct_restrict, ep, SourceEntry, olp, SrcNode.hier, errp) == pr_error )
**			return pr_error;
**	}
*/	else
		if ( Pcommand(ct_restrict, ep, SourceEntry, olp, NULLSTR, errp) == pr_error )
			return pr_error;

	if ( np->nflags & (TERMINAL_NODE|LOCAL_NODE) )
	{
		if ( Pcommand(ct_restrict, SourceEntry, ep, lp, np->/*name*/hier, errp) == pr_error )
			return pr_error;
	}
/*	else
**	if ( np->nflags & LOCAL_NODE )
**	{
**		if ( Pcommand(ct_restrict, SourceEntry, ep, lp, np->hier, errp) == pr_error )
**			return pr_error;
**	}
*/	else
		if ( Pcommand(ct_restrict, SourceEntry, ep, lp, NULLSTR, errp) == pr_error )
			return pr_error;

	/*
	**	For Sun3, these MUST be same in both directions.
	*/

	TokenLength = EncodeNum(buf, np->cost, -1);
	if ( Pcommand(ct_cost, SourceEntry, ep, lp, buf, errp) == pr_error )
		return pr_error;
	if ( Pcommand(ct_cost, ep, SourceEntry, olp, buf, errp) == pr_error )
		return pr_error;

	TokenLength = EncodeNum(buf, np->delay, -1);
	if ( Pcommand(ct_delay, SourceEntry, ep, lp, buf, errp) == pr_error )
		return pr_error;
	if ( Pcommand(ct_delay, ep, SourceEntry, olp, buf, errp) == pr_error )
		return pr_error;

	return pr_ok;
}



/*
**	Read a separator followed by a token (name or number)
**	and return token type.
*/

static T3token
R3token(last_token)
	T3token			last_token;
{
	register char *		s = Buf;
	register int		c;
	register int		repeat_c = 0;
	register FILE *		fd = RStateFd;
	register char *		ep = BufEnd;
	register T3token	token = t3_null;
	register T3tokens	accept;

	DODEBUG(if((int)last_token>=(int)t3_max)Fatal(english("R3token(%d): token range"),last_token));

	accept = Accept3[(int)last_token][State];

	DODEBUG(if(accept==(T3tokens)0&&last_token!=t3_eof)Fatal(english("R3token(%d): accept==0"),last_token));
	DODEBUG(if(Traceflag>=5){Token3Trace(5,last_token,accept);});

	if ( (c = Last_C) != EOF )
	{
		Last_C = EOF;
		goto next;
	}

	for ( ;; )
	{
		if ( (c = getc(fd)) == EOF )
		{
			Trace1(1, "R3token at EOF!");
			if ( token == t3_null )
				return t3_eof;
			goto eof;
		}

next:		*s++ = (char)c;

		if ( repeat_c == 0 || c == repeat_c )
		switch ( c )
		{
		case C3_ALIAS:		c = (int)t3_alias;	goto found;
		case C3_ALIASV:		c = (int)t3_aliasv;	goto found;
		case C3_BYTES:		c = (int)t3_bytes;	goto found;
		case C3_CALLER:		c = (int)t3_caller;	goto found;
		case C3_COMMENT:	c = (int)t3_comment;	goto found;
		case C3_CONNECTOR:	c = (int)t3_connector;	goto found;
		case C3_COST:		c = (int)t3_cost;	goto found;
		case C3_DATE:		c = (int)t3_date;	goto found;
		case C3_DOMAIN:		c = (int)t3_domain;	goto found;
		case C3_DOMHIER:	c = (int)t3_domhier;	goto found;
		case C3_EOF:		c = (int)t3_eof;	goto found;
		case C3_FILTER:		c = (int)t3_filter;	goto found;
		case C3_HANDLER:	c = (int)t3_handler;	goto found;
		case C3_LFLAGS:		c = (int)t3_lflags;	goto found;
		case C3_LINK:		c = (int)t3_link;	goto found;
		case C3_NFLAGS:		c = (int)t3_nflags;	goto found;
		case C3_NODE:		c = (int)t3_node;	goto found;
		case C3_NODEHIER:	c = (int)t3_nodehier;	goto found;
		case C3_OTHDOM:		c = (int)t3_othdom;	goto found;
		case C3_PASSFROM:	c = (int)t3_passfrom;	goto found;
		case C3_PASSTO:		c = (int)t3_passto;	goto found;
		case C3_RECVD:		c = (int)t3_recvd;	goto found;
		case C3_SENT:		c = (int)t3_sent;	goto found;
		case C3_SPARE2:		c = (int)t3_null;	goto found;
		case C3_SPEED:		c = (int)t3_speed;	goto found;
		case C3_SPOOLER:	c = (int)t3_spooler;	goto found;
		case C3_STATE:		c = (int)t3_state;	goto found;
		case C3_TIME:		c = (int)t3_time;	goto found;
		case C3_TOPDOM:		c = (int)t3_topdom;	goto found;
		case C3_XALIAS:		c = (int)t3_xalias;	goto found;

found:			if ( token == t3_null )
			{
				if ( !(accept & (T3tokens)(1L<<c)) )
				{
					if ( accept & T3_EXPECT )
					{
						token = t3_expect;
						goto backout;
					}
					if ( c == (int)t3_eof )
					{
						token = (T3token)c;
						goto backout;
					}
					token = t3_null;
				}
				else
					token = (T3token)c;

				if ( repeat_c )
					repeat_c = 0;
				else
				switch ( (T3token)c )
				{
				case t3_caller:
				case t3_comment:
				case t3_connector:
				case t3_filter:
				case t3_handler:
				case t3_spooler:
					repeat_c = s[-1];
				default:
					break;	/* gcc -Wall */
				}

				if ( s != &Buf[1] )
				{
					c = *--s;
					if ( !IgnoreCRC )
						StCRC = acrc(StCRC, Buf, s - Buf);
					TokenChar += s - Buf;
					Trace3(
						4,
						"Ignore token %ld: \"%s\"",
						TokenCount,
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
				if ( !repeat_c )
backout:				Last_C = *(Uchar *)--s;
eof:				if ( (c = s - Buf) > 0 )
				{
					if ( !IgnoreCRC )
						StCRC = acrc(StCRC, Buf, c);
					TokenChar += c;
					TokenLength = --c;
				}
				else
					TokenLength = 0;
				if ( repeat_c )
				{
					TokenLength--;
					*--s = '\0';
				}
				else
					*s = '\0';
				Trace4(
					4,
					"R3token %ld \"%s\": \"%s\"",
					TokenCount,
					Token3Names[(int)token],
					ExpandString(Buf, TokenLength+1)
				);
				return token;
			}
		}

		if ( s > ep )
		{
			TokenLength = (s - Buf) - 1;
			return t3_error;
		}
	}
}



/*
**	Decode state flags from string.
*/

static void
R3setFlags(np, cp)
	register LinkNode *	np;
	register char *		cp;
{
	register int		c;

	Trace3(3, "R3setFlags(%s) for %s", cp, np->name);

	while ( (c = *cp++) )
	{
		c -= 'A';

		switch ( (S3state)c )
		{
		case s3_dead:		np->lflags |= FL_DEAD;		break;
		case s3_down:		np->lflags |= FL_DOWN;		break;
		case s3_filtered:	np->lflags |= FL_FILTERED;	break;
		case s3_foreign:	np->lflags |= FL_FOREIGN;	break;
		case s3_intermittent:	np->delay = SUN3INTDELAY;	break;
		case s3_local:		np->nflags |= LOCAL_NODE;	break;
		case s3_msg:		np->nflags |= MSG_LINK;		break;
		case s3_msgterminal:	np->nflags |= TERMINAL_NODE;	break;
		default:						break;	/* gcc -Wall */
		}

		Trace2(4, "R3setFlag %s", Flag3Names[c]);
	}
}



/*
**	Set up names for LinkNode from ``buf''.
*/

static void
SetLinkNames(np, buf, namebuf)
	register LinkNode *	np;
	char *			buf;
	char *			namebuf;
{
	Trace4(2, "SetLinkNames(%#lx, %s, %#lx)", (PUlong)np, buf, (PUlong)namebuf);

	Conv34Address(buf, namebuf);			/* Convert to typed form */

	np->length = strlen(namebuf);
	np->name = Malloc(np->length+2);		/* Space for trailing DOMAIN_SEP */

	(void)strcpy(np->name, namebuf);		/* Whole name */
	np->name[np->length] = DOMAIN_SEP;		/* For primary domain match */
	np->name[np->length+1] = '\0';
	np->hier = strchr(np->name, DOMAIN_SEP) + 1;	/* Hierarchy part */

	Trace3(2, "SetLinkNames() -> %s %s", np->name, np->hier);
}



#if	DEBUG >= 2
static void
Token3Trace(level, token, accept)
	int			level;
	T3token			token;
	T3tokens		accept;
{
	register char *		cp;
	register int		i;
	register T3tokens	t;
	static char *		buf;

	if ( buf == NULLSTR )
	{
		int	len = 0;

		for ( i = 0 ; i < (int)t3_n ; i++ )
			len += strlen(Token3Names[i]);

		buf = Malloc(len+i);
	}

	cp = buf;
	*cp++ = ' ';

	for ( t = 1, i = 0 ; i < (int)t3_n ; i++, t <<= 1 )
		if ( accept & t )
		{
			cp = strcpyend(cp, Token3Names[i]);
			*cp++ = ',';
		}

	*--cp = '\0';

	Trace(level, "R3token(%s) <=%s", Token3Names[(int)token], buf);
}
#endif	/* DEBUG >= 2 */
#endif	/* SUN3 == 1 */
