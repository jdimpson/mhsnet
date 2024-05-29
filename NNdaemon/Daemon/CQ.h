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
**	Queue for blocked control messages.
*/

typedef struct CQel	CQel;
struct CQel
{
	CQel *	cql_next;
	int	cql_size;
	char	cql_data[MAXCNTLDATAZ];
};

typedef CQel *		CQl_p;

Extern CQl_p		CQfreelist;

typedef struct
{
	CQl_p	cq_first;
	CQl_p *	cq_last;
}
			CQhead;

typedef CQhead *	CQh_p;

extern CQhead		ControlQ[];

extern bool		flushCq _FA_((int, bool));
