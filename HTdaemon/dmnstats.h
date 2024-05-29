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
**	Daemon statistics.
*/

#if	VCDAEMON_STATS == 1

typedef Ulong		DmnStat;

enum
{
	ds_vcdata, ds_vcpartio,
	ds_messages, ds_data,
	ds_nstats
};

#define	DS_DATA		(int)ds_data		/* read/write data in messages */
#define	DS_MESSAGES	(int)ds_messages	/* read/write messages */
#define	DS_VCDATA	(int)ds_vcdata		/* read/write data on VC */
#define	DS_VCPARTWRITE	(int)ds_vcpartio	/* partial write on VC */
#define	DS_VCREAD0	(int)ds_vcpartio	/* read 0 on VC */
#define	DS_NSTATS	(int)ds_nstats		/*  */

#define	DmnStats	Status.st_dmnstats

#ifdef	DMNSTATSDATA

char *			DmnStNames[DS_NSTATS] =
{
	english("Circuit bytes"),
	english("Circuit partial i/o"),
	english("Messages"),
	english("Message bytes"),
};

#define	DMNSTATSNAME(S)		DmnStNames[S]

#endif	/* DMNSTATSDATA */

#define	DMNSTATSINC(S)		DmnStats[S]++
#define	DMNSTATSINCC(S,C)	DmnStats[S] += C

#else	/* VCDAEMON_STATS */

#define	DMNSTATSINC(S)
#define	DMNSTATSINCC(S,C)

#endif	/* VCDAEMON_STATS */

extern void	prtdmnstats _FA_((FILE *, char *));
