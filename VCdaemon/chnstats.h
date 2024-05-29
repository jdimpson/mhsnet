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
**	Channel receiver statistics.
*/

enum
{
	rch_messages, rch_data,
	rch_baddata, rch_badaddr, rch_badsom,
	rch_badeom, rch_unxeom, rch_xeom,
	rch_restart, rch_endrestart, rch_duprestart,
	rch_dupdata, rch_abort, rch_datanak,
	rch_nstats
};

typedef Ulong	RCHstat[(int)rch_nstats];

#ifdef	CHNSTATSDATA

char *		RCHstNames[(int)rch_nstats] =
{
	english("Messages received"),
	english("Data bytes received"),
	english("Data packets received for inactive channel"),
	english("Data packets received off end of message"),
	english("Badly formatted SOM packets"),
	english("Badly formatted EOM packets"),
	english("Unexpected EOM packets"),
	english("Extra EOM packets"),
	english("Active messages restarted"),
	english("Ended messages restarted"),
	english("Possible duplicated messages"),
	english("Duplicate data packets received"),
	english("Active messages aborted"),
	english("NAKs for DATA packets")
};

#endif	/* CHNSTATSDATA */


/*
**	Channel transmitter statistics.
*/

enum
{
	wch_messages, wch_data,
	wch_badnak, wch_unxnak, wch_badsoma, wch_badeoma, wch_unxeoma, wch_unxsoma,
	wch_restart, wch_duprestart, wch_datanak,
	wch_nstats
};

typedef Ulong	WCHstat[(int)wch_nstats];

#ifdef	CHNSTATSDATA

char *		WCHstNames[(int)wch_nstats] =
{
	english("Messages transmitted"),
	english("Data bytes transmitted"),
	english("Malformed NAK packets"),
	english("Unexpected NAK packets"),
	english("Malformed SOMA packets"),
	english("Malformed EOMA packets"),
	english("Unexpected EOMA packets"),
	english("Unexpected SOMA packets"),
	english("Restarted messages"),
	english("Restarted messages already transmitted"),
	english("NAKs for DATA packets")
};

#endif	/* CHNSTATSDATA */

extern void	prtchnstats _FA_((FILE *, bool, char *));
