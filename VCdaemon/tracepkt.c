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


#define	FILE_CONTROL
#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"packet.h"


#if	PROTO_STATS >= 1

/*
**	Trace data.
*/

void
_TraceData(desc, size, data)
	char *		desc;
	int		size;
	char *		data;
{
	register char *	cp;
	register int	n;
	static char	cont[] = "...";
#	define		MAXDATALEN	37

	cp = EmptyString;

	if ( (n = size) > MAXDATALEN )
	{
		n = MAXDATALEN;
		cp = cont;
	}

	data = ExpandString(data, n);

	if ( (int)strlen(data) > MAXDATALEN )
	{
		data[MAXDATALEN] = '\0';
		cp = cont;
	}

	Trace(0, "%28s %6d [%s]%s", desc, size, data, cp);
}



/*
**	Trace a packet.
*/

void
_TracePkt(direction, pkt, comment, repeat)
	PktDir		direction;
	Uchar *		pkt;
	char *		comment;
	bool		repeat;
{
	char *		dir;
	char *		type;
	char *		datastring;
	int		size;
	int		datasize;
	Ulong		thistime;
	Ulong		elapsedtime;
	static Ulong	lasttime;
	static char *	lastcomment;

	extern char	ReaderName[];
	/* extern char	WriterName[]; */

/*	static char	header[] = "elapse <<>> ch type size    address [bytes...] comment\n";	*/
	static char	format[] = "%6lu %s %2d%5s%5d%11lu [%.40s] %s\n";
#	define		MAXDATASTRLEN			      40

	if ( comment == lastcomment && !repeat )
		return;
	lastcomment = comment;

	thistime = millisecs();
	if ( lasttime > thistime )
		elapsedtime = thistime + (MAX_ULONG - lasttime) + 1;
	else
	if ( lasttime == 0 )
		elapsedtime = 0;
	else
		elapsedtime = thistime - lasttime;
	lasttime = thistime;

	if ( Name == ReaderName )
		switch ( direction )
		{
		case dir_vcin:		dir = "<  R";	break;
		case dir_vcout:		dir = " >ER";	break;
		case dir_pipin:		dir = "<< R";	break;
		case dir_pipout:	dir = ">> R";	break;
		}
	else /* Name == WriterName */
		switch ( direction )
		{
		case dir_vcin:		dir = "WE< ";	break;
		case dir_vcout:		dir = "W  >";	break;
		case dir_pipin:		dir = "W <<";	break;
		case dir_pipout:	dir = "W >>";	break;
		}

	switch ( G_PKTTYPE(pkt) )
	{
	case PKTDATATYPE:	type = EmptyString;	break;
	case PKTNAKTYPE:	type = "NAK";		break;
	case PKTSOMTYPE:	type = "SOM";		break;
	case PKTSOMATYPE:	type = "SOMA";		break;
	case PKTEOMTYPE:	type = "EOM";		break;
	case PKTEOMATYPE:	type = "EOMA";		break;
	case PKTEOTTYPE:
		switch ( G_PKTCHAN(pkt) )
		{
		case LEOT:	type = "LEOT";		break;
		case REOT:	type = "REOT";		break;
		default:	type = "?EOT";		break;
		}
		break;
	case PKTMQETYPE:
		switch ( G_PKTCHAN(pkt) )
		{
		case MQE:	type = "MQE";		break;
		case SYNC:	type = "SYNC";		break;
		case IDLE:	type = "IDLE";		break;
		case CTRL:	type = "CTRL";		break;
		}
		break;
	}

	if ( (size = G_PKTSIZE(pkt)) > MAXDATASTRLEN )
		datasize = MAXDATASTRLEN;
	else
		datasize = size;

	datastring = ExpandString((char *)A_PKTDATA(pkt), datasize);

#	if	FCNTL != 1 || O_APPEND == 0
	(void)fseek(TraceFd, (long)0, 2);
#	endif	/* FCNTL != 1 || O_APPEND == 0 */

	(void)fprintf
	(
		TraceFd, format,
		(PUlong)elapsedtime,
		dir,
		G_PKTCHAN(pkt),
		type,
		size,
		(PUlong)G_PKTADDR(pkt),
		datastring,
		comment
	);

	(void)fflush(TraceFd);
}

#endif	/* PROTO_STATS >= 1 */
