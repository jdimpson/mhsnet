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
**	Queue for packets awaiting transmission
*/

#ifndef	PKTHDR
#include	"Packet.h"
#endif	/* PKTHDR */


typedef struct PQel	PQel;
struct PQel
{
	PQel *	pql_next;
	char *	pql_pktp;
	int	pql_size;
	Cntlpkt	pql_pkt;
};

typedef PQel *		PQl_p;

Extern PQl_p		PQfreelist;

typedef struct
{
	PQl_p	pq_first;
	PQl_p *	pq_last;
}
			PQhead;

/*
**	Two queues:	[0] for control packets
**			[1] for data packets
*/

#define	PCNTRLQ		0
#define	PDATAQ		1
#define	NPKTQ		2

Extern PQhead		PktQ[NPKTQ];
