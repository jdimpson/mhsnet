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


#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"



/*
**	Move commands from one header to another.
*/

void
MoveCmds(from, to)
	register CmdHead *	from;
	register CmdHead *	to;
{
	Trace3(1, "MoveCmds(%#lx, %#lx)", (PUlong)from, (PUlong)to);

	if ( (*to->ch_last = from->ch_first) == (Cmd *)0 )
		return;
	to->ch_last = from->ch_last;
	to->ch_count += from->ch_count;

	InitCmds(from);
}
