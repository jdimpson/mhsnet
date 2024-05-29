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
**	Queue handling routines.
*/

#include	"global.h"
#include	"debug.h"

#include	"driver.h"
#include	"AQ.h"
#include	"CQ.h"
#include	"PQ.h"

extern int	Psendc(int, char *, int);


/*
**	Queue an "action" for later execution
*/

void
qAction(action, arg)
	vFuncp		action;
	AQarg		arg;
{
	register AQl_p	aqlp;

	if ( NoAction )
		return;

	if ( (aqlp = AQfreelist) != (AQl_p)0 )
		AQfreelist = aqlp->aql_next;
	else
		aqlp = (AQl_p)Malloc(sizeof(*aqlp));

	aqlp->aql_next = (AQl_p)0;
	aqlp->aql_funcp = action;
	aqlp->aql_arg.qu_arg = arg;

	*ActionQ.aq_last = aqlp;
	ActionQ.aq_last = &aqlp->aql_next;
}



/*
**	Send a control message,
**	else queue the blocked message for later transmission.
*/

void
qControl(chan, data, size)
	int		chan;
	register char *	data;
	register int	size;
{
	register char *	cp;
	register CQl_p	cqlp;
	register CQh_p	cqhp;

	Trace4(2, "qControl \"%s\" size %d chan %d", ExpandString(data, size), size, chan);

	if ( !flushCq(chan, false) )
	{
		switch ( Psendc(chan, data, size) )
		{
		case -1:
			Fatal1("error return from \"Psendc\"");
		case 1:
			return;
		}

		Trace1(2, "qControl Psendc failed");
	}

	/*
	**	Channel is busy, so queue the message for later transmission by "fillPkt()".
	*/

	if ( (cqlp = CQfreelist) == (CQl_p)0 )
		cqlp = (CQl_p)Malloc(sizeof(*cqlp));
	else
		CQfreelist = cqlp->cql_next;

	cqlp->cql_next = (CQl_p)0;
	cqlp->cql_size = size;

	cp = cqlp->cql_data;

	while ( --size >= 0 )
		*cp++ = *data++;

	cqhp = &ControlQ[chan];

	*cqhp->cq_last = cqlp;
	cqhp->cq_last = &cqlp->cql_next;
}



/*
**	Queue a data packet for later transmission.
**	NB: data is not copied -> problem for slow transmission.
*/

void
qPkt(pktp, size)
	char *		pktp;
	int		size;
{
	register PQl_p	pqlp;
	register PQl_p *nextp;

	for
	(
		nextp = &PktQ[PDATAQ].pq_first ;
		(pqlp = *nextp) != (PQl_p)0 ;
		nextp = &pqlp->pql_next
	)
	{
		/*
		**	This may be a re-transmission of a packet
		**	that has not yet been sent!
		*/
		if ( pqlp->pql_pktp == pktp )
		{
			Trace2(2, "qPkt moved packet size %d", size);
			/*
			**	Extract element from queue
			*/
			if ( &pqlp->pql_next == PktQ[PDATAQ].pq_last )
				PktQ[PDATAQ].pq_last = nextp;
			*nextp = pqlp->pql_next;
			/*
			**	Fix element
			*/
			pqlp->pql_next = (PQl_p)0;
			pqlp->pql_size = size;
			/*
			**	Put back on end
			*/
			*PktQ[PDATAQ].pq_last = pqlp;
			PktQ[PDATAQ].pq_last = &pqlp->pql_next;
			return;
		}
	}

	Trace2(2, "qPkt size %d", size);

	if ( (pqlp = PQfreelist) != (PQl_p)0 )
		PQfreelist = pqlp->pql_next;
	else
		pqlp = (PQl_p)Malloc(sizeof(*pqlp));

	pqlp->pql_pktp = pktp;
	pqlp->pql_next = (PQl_p)0;
	pqlp->pql_size = size;

	*PktQ[PDATAQ].pq_last = pqlp;
	PktQ[PDATAQ].pq_last = &pqlp->pql_next;
}



/*
**	Queue a control packet for later transmission.
**	Must copy the data.
*/

void
qCpkt(pktp, size)
	char *		pktp;
	int		size;
{
	register PQl_p	pqlp;

	Trace2(2, "qCpkt size %d", size);

	if ( (pqlp = PQfreelist) != (PQl_p)0 )
		PQfreelist = pqlp->pql_next;
	else
		pqlp = (PQl_p)Malloc(sizeof(*pqlp));

	pqlp->pql_next = (PQl_p)0;
	pqlp->pql_pkt = *(Cntlpkt *)pktp;
	pqlp->pql_pktp = (char *)&pqlp->pql_pkt;
	pqlp->pql_size = size;

	*PktQ[PCNTRLQ].pq_last = pqlp;
	PktQ[PCNTRLQ].pq_last = &pqlp->pql_next;
}



/*
**	If there are any blocked control messages to send,
**	try sending them again and return TRUE,
**	else return FALSE.
*/

bool
flushCq(chan, drop)
	int		chan;
	bool		drop;
{
	register CQl_p	cqlp;
	register CQh_p	cqhp = &ControlQ[chan];
	register bool	val = false;

	Trace2(2, "flushCq chan %d", chan);

	while ( (cqlp = cqhp->cq_first) != (CQl_p)0 )
	{
		if ( !drop )
			switch ( Psendc(chan, cqlp->cql_data, cqlp->cql_size) )
			{
			case -1:
				Fatal1("error return from \"Psendc\"");
			case 0:
				Trace1(2, "flushCq Psendc failed");
				return true;
			}

		if ( (cqhp->cq_first = cqlp->cql_next) == (CQl_p)0 )
			cqhp->cq_last = &cqhp->cq_first;

		cqlp->cql_next = CQfreelist;
		CQfreelist = cqlp;
		val = true;
	}

	return val;
}
