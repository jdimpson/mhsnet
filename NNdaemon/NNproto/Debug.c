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
**	Debugging routines
*/

#define	STDIO

#include	"global.h"

#include	"Pconfig.h"
#include	"Packet.h"
#include	"Proto.h"
#include	"Debug.h"


Time_t			Debug_ticks;
extern int		Ptraceflag;			/* Defined in ``main.c'' */
FILE *			PtraceFd	= NULL;		/* Initialised in InitPtrace() */

typedef struct 
{
	char	lp_pkt[PKTLOGLEN];
	char	lp_ident;
}
			LogPkt;

typedef struct
{
	LogPkt	lg_pkt;
	Timo_t	lg_ticks;
}
			Pktlog;

typedef Pktlog *	Log_p;

#define	lg_ident	lg_pkt.lp_ident

static Pktlog		log[PKTHIST];
static Log_p		inlog		= log;
static short		logcount;

#define			Fprintf		(void)fprintf

#define	TYPE		((pktp->pkt_hdr[PKTHDR]>>PKTTYP_S)&PKTTYP_M)
#define	CHANNEL		((pktp->pkt_hdr[PKTHDR]>>PKTCHN_S)&PKTCHN_M)
#define	SEQUENCE	((pktp->pkt_hdr[PKTHDR]>>PKTSEQ_S)&PKTSEQ_M)
#define	DATA0		BYTE(pktp->pkt_data[0])


void
Ptracepkt(pktp, ident, t)
	Pkt_p		pktp;
	char		ident;
	Timo_t		t;
{
	register char * cp;
	register int	size;
	register char *	type;

	switch ( TYPE )
	{
	default:	/* gcc -Wall */
	case PKTDATATYP:	type = "   ";	break;
	case PKTACKTYP:		type = "ack";	break;
	case PKTNAKTYP:		type = "nak";	break;
	case PKTCNTLTYP:	type = "ctl";
		if ( DATA0 & CONTROL )
			switch ( DATA0 )
			{
			case TIMEOUT:	type = "TIM";	break;
			case XMT_RESET:	type = "XMR";	break;
			case ACK_RESET:	type = "AKR";	break;
			case REQ_RESET:	type = "RQR";	break;
			case IDLE:	type = "IDL";	break;
			}
		else
			switch ( DATA0 )
			{
			case 1:		type = "SOM";	break;
			case 2:		type = "SMA";	break;
			case 3:		type = "EOM";	break;
			case 4:		type = "EMA";	break;
			case 5:		type = "MQE";	break;
			}
		break;
	}

#	if	defined(NNstrpr) || defined(NNstrpr2)
	size = BYTE(pktp->pkt_hdr[PKTSIZE]);
#	endif	/* defined(NNstrpr) || defined(NNstrpr2) */
#	ifdef	ENproto
	size = BYTE(pktp->pkt_hdr[PKTSIZE0]);
	size |= BYTE(pktp->pkt_hdr[PKTSIZE1]) << 8;
#	endif	/* ENproto */
#	ifdef	PNproto
	size = BYTE(pktp->pkt_hdr[PKTSIZE0]);
	size |= (BYTE(pktp->pkt_hdr[PKTSIZE1])&PKTSIZ_M) << (8-PKTSIZ_S);
#	endif	/* PNproto */

	Fprintf
	(
		 PtraceFd
		,"%4d %s N:%d,Q:%d,T:%s,Z:%-3d ["
		,t
		,ident==PLOGIN?"RECV":ident==PLOGOUT?"xque":"xmit"
		,CHANNEL
		,SEQUENCE
		,type
		,size
	);

	size += PKTHDRZ + PKTCRCZ;

	if ( size > PKTLOGLEN )
		size = PKTLOGLEN;

	for ( cp = (char *)pktp ; --size >= 0 ; )
		Fprintf(PtraceFd, " %03o", BYTE(*cp++));

	Fprintf(PtraceFd, " ]\n");
}




void
Plogpkt(pktp, ident)
	Pkt_p		pktp;
	char		ident;
{
	register Time_t	ticksnow;

	ticksnow = millisecs();
	inlog->lg_ticks = ticksnow - Debug_ticks;
	Debug_ticks = ticksnow;

	if ( Ptraceflag >= 2 )
	{
		Ptracepkt(pktp, ident, inlog->lg_ticks);
		(void)fflush(PtraceFd);
		return;
	}

	inlog->lg_pkt = *(LogPkt *)pktp;

	inlog->lg_ident = ident;

	if ( ++inlog >= &log[PKTHIST] )
		inlog = log;

	if ( ++logcount > PKTHIST )
		logcount = PKTHIST;
}



void
Pdumphist(s)
	char *	s;
{
	register int	first = 1;
	register Log_p	outlog;

	if ( (outlog = inlog - logcount) < log )
		outlog += PKTHIST;

	while ( logcount )
	{
		if ( first )
		{
			first = 0;
			Fprintf(PtraceFd, "%s - last %d packets\n", s, logcount);
		}
		logcount--;
		Ptracepkt((Pkt_p)outlog, outlog->lg_ident, outlog->lg_ticks);
		if ( ++outlog >= &log[PKTHIST] )
			outlog = log;
	}

	if ( first && Ptraceflag >= 2 )
		Fprintf(PtraceFd, "%s\n", s);

	(void)fflush(PtraceFd);
}



void
Ptracehdrs(badhdrs)
	int	badhdrs;
{
	Fprintf(PtraceFd, "%d bad header bytes\n", badhdrs);
}



/*
**	Initialise `PtraceFd'.
*/

void
InitPtrace()
{
	PtraceFd = stderr;
}
