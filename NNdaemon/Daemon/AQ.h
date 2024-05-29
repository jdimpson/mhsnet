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
**	Queue for "actions" awaiting execution
*/


/*
**	Unions cannot be operands of CASTS (ho hum), so...
*/

typedef long		AQarg;

typedef union
{
	AQarg	qu_arg;		/* Better be the biggest */
	long	qu_long;
	int	qu_int;
	char *	qu_cp;
}
			Arg_t;
/*
**	An action queue element.
*/

typedef struct AQel	AQel;
struct AQel
{
	AQel *	aql_next;
	vFuncp	aql_funcp;
	Arg_t	aql_arg;
};

typedef AQel *		AQl_p;

Extern AQl_p		AQfreelist;
Extern bool		NoAction;	/* Queue is being flushed */

typedef struct
{
	AQl_p	aq_first;
	AQl_p *	aq_last;
}
			AQhead;

extern AQhead		ActionQ;

/*
**	Daemon functions called via the "action" queue.
*/

extern void
		EndMessage _FA_((AQarg)),
		MakeTemp _FA_((AQarg)),
		NewMessage _FA_((AQarg)),
		PosnStream _FA_((AQarg)),
		qAction _FA_((vFuncp, AQarg));
