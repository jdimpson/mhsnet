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


#include	"global.h"
#include	"address.h"
#undef	Extern
#define	Extern
#define	ExternU
#include	"expand.h"

static char *	selecta _FA_((char));
static int	selectp _FA_((char));
static int	selectx _FA_((char));



/*
**	Expand arguments containing any '&x' sequences;
**	change arguments containing any '%xstring%' sequences.
*/

void
ExpandArgs(to, app)
	VarArgs	*	to;
	char **		app;
{
	register char *	ap;
	register char *	dp;
	register char *	sp;
	register char *	cp;
	register int	size;
	char *		string;
	bool		hadsub;

	if ( app == (char **)0 )
		return;

	while ( (ap = *app++) != NULLSTR )
	{
		if ( *ap == '\0' )
			continue;

		dp = ap;
		size = 1;
		hadsub = false;

		while ( (dp = strchr(dp, '&')) != NULLSTR )
			if ( (sp = selecta(*++dp)) != NULLSTR )
			{
				size += strlen(sp);
				dp++;
			}

		size += strlen(ap);

		string = sp = Malloc(size);

		while ( *ap != '\0' && (dp = strpbrk(ap, "&%")) != NULLSTR )
		{
			hadsub = true;

			if ( (size = dp-ap) )
				sp = strncpy(sp, ap, size) + size;

			if ( *dp++ == '&' )
			{
				if ( (cp = selecta(*dp)) != NULLSTR )
				{
					sp = strcpyend(sp, cp);
					dp++;
				}
				else
				{
					*sp++ = '&';
					*sp++ = *dp++;
				}
			}
			else
			{
				register int	negate;
				register int	testex;

				testex = 0;
				size = *dp;

				if ( size == '+' || size == '-' )
				{
					testex = 1;
					negate = (size == '-');
					size = *++dp;
				}
				else
				if ( (negate = (size == '!')) )
					size = *++dp;

				cp = &dp[1];

				if
				(
					(ap = strchr(cp, '%')) != NULLSTR
					&&
					(size = testex?selectx(size):selectp(size)) != 2
				)
				{
					if ( size != negate )
					{
						while
						(
							(dp = strchr(cp, '&')) != NULLSTR
							&&
							dp < ap
						)
						{
							if ( (size = dp-cp) )
								sp = strncpy(sp, cp, size) + size;
							if ( (cp = selecta(*++dp)) != NULLSTR )
							{
								sp = strcpyend(sp, cp);
								dp++;
							}
							else
							{
								*sp++ = '&';
								*sp++ = *dp++;
							}
							cp = dp;
						}
						if ( (size = ap-cp) )
							sp = strncpy(sp, cp, size) + size;
					}
					dp = ap + 1;
				}
				else
				{
					*sp++ = '%';
					if ( testex )
					{
						if ( negate )
							*sp++ = '-';
						else
							*sp++ = '+';
					}
					else
					if ( negate )
						*sp++ = '!';
				}
			}

			ap = dp;
		}

		(void)strcpy(sp, ap);

		if ( !hadsub || (int)strlen(string) > 0 )
			NEXTARG(to) = string;
		else
			free(string);
	}
}



/*
**	String test: NULLSTR - nomatch.
**
**	(Include changes in `selectx()' below.)
*/

static char *
#ifdef	ANSI_C
selecta(char c)
#else	/* ANSI_C */
selecta(c)
	char		c;
#endif	/* ANSI_C */
{
	char *		cp;
	static char	str[ULONG_SIZE];

	switch ( c )
	{
	case 'D':	cp = DataName; break;
	case 'E':	cp = EnvError; break;
	case 'F':	cp = SenderName; break;
	case 'H':	cp = ExHomeAddress; break;
	case 'L':	cp = ExLinkAddress; break;
	case 'O':	cp = ExSourceAddress; break;
	case 'S':	(void)sprintf(str, "%lu", (PUlong)DataSize); cp = str; break;
	case 'T':	(void)sprintf(str, "%lu", (PUlong)StartTime); cp = str; break;
	case 'U':	cp = UserName; break;
	case '&':	cp = "&"; break;
	case '%':	cp = "%"; break;
	default:	return NULLSTR;
	}

	return (cp == NULLSTR) ? EmptyString : cp;
}



/*
**	Boolean test: 0 - false; 1 - true; 2 - nomatch.
*/

static int
#ifdef	ANSI_C
selectp(char c)
#else	/* ANSI_C */
selectp(c)
	char	c;
#endif	/* ANSI_C */
{
	switch ( c )
	{
	case 'A':	return (int)Ack;
	case 'B':	return (int)Broadcast;
	case 'D':	return DupNode != NULLSTR;
	case 'H':	return (int)StringAtHome(ExSourceAddress);
	case 'L':	if
			(
				ExLinkAddress == NULLSTR
				||
				ExLinkAddress[0] == '\0'
				||
				AddressMatch(ExSourceAddress, ExLinkAddress)
			)
				return 0;
			return 1;
	case 'Q':	return (int)ReqAck;
	case 'R':	return (int)Returned;
	case 'T':	return TruncNode != NULLSTR;
	}

	return 2;
}



/*
**	Boolean null string test: 0 - false; 1 - true; 2 - nomatch.
*/

static int
#ifdef	ANSI_C
selectx(char c)
#else	/* ANSI_C */
selectx(c)
	char	c;
#endif	/* ANSI_C */
{
	switch ( c )
	{
	case 'D':	return DataName != NULLSTR;
	case 'E':	return EnvError != NULLSTR;
	case 'F':	return SenderName != NULLSTR;
	case 'H':	return ExHomeAddress != NULLSTR;
	case 'L':	return ExLinkAddress != NULLSTR;
	case 'O':	return ExSourceAddress != NULLSTR;
	case 'S':	return DataSize != 0;
	case 'T':	return StartTime != 0;
	case 'U':	return UserName != NULLSTR;
	case '&':	return 1;
	case '%':	return 1;
	}

	return 2;
}
