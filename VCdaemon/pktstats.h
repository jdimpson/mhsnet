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
**	Virtual circuit protocol statistics.
*/

#if	PROTO_STATS == 1

typedef Ulong	PktStat;

enum
{
	ps_skipdata, ps_skipbyte, ps_badsize, ps_baddcrc, ps_badtype,
	ps_rpkts, ps_xpkts,
	ps_nstats
};

#define	PS_SKIPDATA	(int)ps_skipdata	/* Software error */
#define	PS_SKIPBYTE	(int)ps_skipbyte	/* Bad header / synchronisation */
#define	PS_BADSIZE	(int)ps_badsize		/* Size out-of-range */
#define	PS_BADDCRC	(int)ps_baddcrc		/* Data CRC error */
#define	PS_BADTYPE	(int)ps_badtype		/* Type out-of-range */
#define	PS_RPKTS	(int)ps_rpkts		/* Packets received */
#define	PS_XPKTS	(int)ps_xpkts		/* Packets transmitted */
#define	PS_NSTATS	(int)ps_nstats		/*  */

#define	PktStats	Status.st_pktstats

#ifdef	PKTSTATSDATA

char *		PktStNames[PS_NSTATS] =
{
	english("Software error"),
	english("Bad header / synchronisation"),
	english("Size out-of-range"),
	english("Data CRC error"),
	english("Type out-of-range"),
	english("Packets received"),
	english("Packets transmitted"),
};

#define	PKTSTATSNAME(S)		PktStNames[S]

#endif	/* PKTSTATSDATA */

#define	PKTSTATSINC(S)		PktStats[S]++
#define	PKTSTATSINCC(S,C)	PktStats[S] += C

#else	/* PROTO_STATS */

#define	PKTSTATSINC(S)
#define	PKTSTATSINCC(S,C)

#endif	/* PROTO_STATS */

extern void	prtpktstats _FA_((FILE *, char *));
