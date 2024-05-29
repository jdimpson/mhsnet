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


#define	STDIO

#include	"global.h"
#include	"call.h"



/*
**	Compile an operation.
*/

Op *
op(opcode)
	int	opcode;
{
	Op *	o;

	o = Talloc(Op);
	o->code = opcode;
	o->op1.str = (char *)0;
	o->op2.str = (char *)0;
	o->next = (Op *)0;
	o->lab = (Symbol *)0;

	if ( CallScript.head == (Op *)0 )
		CallScript.head = o;

	return o;
}
