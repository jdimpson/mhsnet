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
**	Compile a pattern.
*/

Pat *
pat(psym, id)
	Symbol *	psym;
	Symbol *	id;
{
	register Pat *	p;

	p = Talloc(Pat);
	p->next = (Pat *)0;
	p->pattern = psym->val.str; 
	p->comp_pat = (char *)0;
	p->nextsym = id;

	return p;
}
