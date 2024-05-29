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
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"

#define	COMMENT_DATA
#include	"commands.h"


/*
**	Structure for comment records;
*/

typedef struct
{
	char *		sv_begin;
	char *		sv_end;
	SiteInfo *	sv_info;
}
		SiteVec;

static SiteVec	Parts[NSINS+1];

int		cmprec _FA_((const void *, const void *));
static char *	MakeComment _FA_((bool));
static void	SetCmtNmLengths _FA_((void));



/*
**	Check if any comment field has changed, and if so make up new one.
*/

void
CheckComment()
{
	register char *	cp;
	static char	buf[DATETIMESTRLEN];

	Trace1(1, "CheckComment");

#	if	CHECK_LICENCE != 1
	if ( HomeComment != NULLSTR && !NewComment )
		return;
#	endif	/* CHECK_LICENCE != 1 */

	if ( SiteInfoNames[(int)si_edited].si_namelength == 0 )
		SetCmtNmLengths();

	cp = MakeComment(true);

	if ( HomeComment == NULLSTR || strccmp(cp, HomeComment) != STREQUAL )
	{
		Trace3(1, "CheckComment: \"%s\" != \"%s\"", HomeComment, cp);

		SiteInfoNames[(int)si_edited].si_value = DateTimeStr(Time, buf);
		SiteInfoNames[(int)si_edited].si_length = DATETIMESTRLEN - 1;

		HomeComment = MakeComment(false);
	}

	free(cp);

#	if	CHECK_LICENCE
	LicenceNumber = newnstr
			(
				SiteInfoNames[(int)si_licence].si_value,
				SiteInfoNames[(int)si_licence].si_length
			);
	Trace2(1, "LicenceNumber = \"%s\"", LicenceNumber);
#	else	/* CHECK_LICENCE */
	NewComment = false;
#	endif	/* CHECK_LICENCE */
}



int
#if	ANSI_C
cmprec(const void *vp1, const void *vp2)
#else	/* ANSI_C */
cmprec(vp1, vp2)
	char *	vp1;
	char *	vp2;
#endif	/* ANSI_C */
{
	if ( ((SiteVec *)vp1)->sv_begin > ((SiteVec *)vp2)->sv_begin )
		return 1;
	if ( ((SiteVec *)vp1)->sv_begin < ((SiteVec *)vp2)->sv_begin )
		return -1;
	return 0;
}



/*
**	Make SiteInfo field from SiteInfo records.
*/

static char *
MakeComment(whinge)
	bool			whinge;
{
	register SiteInfo *	sp;
	register char *		cp;
	register int		i;
	register int		c;
	register int		len;
	char *			result;
	static char		errf[] = english("No value for site info \"%s\" field.");

	Trace2(2, "MakeComment(%s)", whinge?"whinge":EmptyString);

	len = NSINS * (LenInfoSep + 1);

	for ( sp = SiteInfoNames, i = NSINS ; --i >= 0 ; sp++ )
		len += sp->si_length + sp->si_namelength;

	result = cp = Malloc(len+1);

	for ( sp = SiteInfoNames, i = NSINS ; --i >= 0 ; sp++ )
	{
		if ( sp->si_value == NULLSTR )
		{
			if ( !whinge || i == 0 )
				continue;

			if ( sp->si_mandatory )
				Error(errf, sp->si_name);

			continue;
		}

		if ( sp->si_type == ct_licence )
			continue;	/* No longer included */

		len = sp->si_length;
		cp = strcpyend(cp, sp->si_name);
		cp = strcpyend(cp, StInfoSep);
		c = sp->si_value[len];
		sp->si_value[len] = '\0';
		cp = strcpyend(cp, sp->si_value);

		Trace4(3, "MakeComment ==> %s%s%s", sp->si_name, StInfoSep, sp->si_value);

		sp->si_value[len] = c;
		*cp++ = '\n';
	}

	*cp++ = '\0';

	if ( (cp - result) > TOKEN_SIZE )
		Error(english("Site Info too long"));

	Trace2(2, "MakeComment ==> %s", result);

	return result;
}



/*
**	Set lengths of ID strings.
*/

static void
SetCmtNmLengths()
{
	register SiteInfo *	sp;
	register int		i;

	Trace1(3, "SetCmtNmLengths()");

	for ( sp = SiteInfoNames, i = NSINS ; --i >= 0 ; sp++ )
		sp->si_namelength = strlen(sp->si_name);
}



/*
**	Split HomeComment field into component records.
*/

void
SplitComment()
{
	register SiteVec *	vp;
	register int		i;
	register SiteInfo *	sp;
	register char *		cp;
	register int		nrecs;

	Trace2(2, "SplitComment: \"%s\"", HomeComment);

	if ( SiteInfoNames[(int)si_edited].si_namelength == 0 )
		SetCmtNmLengths();

	while ( *HomeComment == '\n' )
		HomeComment++;

	/*
	**	Find each record.
	*/

	for ( vp = Parts, sp = SiteInfoNames, i = NSINS ; --i >= 0 ; sp++ )
	{
		char	buf[100];

		(void)strcpy(strcpyend(buf, sp->si_name), StInfoSep);

		if ( (cp = StringMatch(HomeComment, buf)) == NULLSTR )
			continue;

		Trace3(4, "Split at [%.*s]", 50, ExpandString(cp, -1));
		sp->si_value = cp + sp->si_namelength + LenInfoSep;
		vp->sv_begin = cp;
		vp->sv_info = sp;
		vp++;
	}

	/*
	**	Mark the end.
	*/

	vp->sv_begin = HomeComment + strlen(HomeComment);
	nrecs = vp - Parts;

	/*
	**	Sort the records according to occurence.
	*/

	qsort((char *)Parts, nrecs + 1, sizeof(SiteVec), cmprec);

	/*
	**	Mark the ends of each record.
	*/

	for ( vp = Parts, i = nrecs ; --i >= 0 ; vp++ )
	{
		cp = (vp+1)->sv_begin - 1;
		while ( *cp == '\n' )
			cp--;
		cp++;
		vp->sv_end = cp;
		Trace5(
			4, "Mark end [%.*s] at [%.*s]",
			8, vp->sv_begin,
			20, ExpandString(cp, -1)
		);
	}

	/*
	**	Match records with SiteInfoNames and set lengths of each value.
	*/

	for ( vp = Parts, i = nrecs ; --i >= 0 ; vp++ )
	{
		sp = vp->sv_info;
		sp->si_length = vp->sv_end - sp->si_value;
		Trace6(
			3,
			"%-14s [%2d] \"%*.*s\"",
			sp->si_name,
			sp->si_length,
			sp->si_length,
			sp->si_length,
			sp->si_value
		);
	}

	if ( (i = Parts[0].sv_begin - HomeComment) > 0 )
		Warn(english("Unrecognised record in comment field: \"%.*s\""), i, HomeComment);
}
