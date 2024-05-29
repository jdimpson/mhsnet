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
**	symbol table handling functions
*/

#define	STDIO

#include	"global.h"
#include	"call.h" 
#include	"debug.h"
#include	"grammar.h"


#define	HASHSIZE	256		/* 256 preferred for table lookup hash */

static Symbol *	symtab[HASHSIZE];	/* Symbol hash table */


/*
**	Pseudorandom permutation of integers 0-255
*/

static Uchar	randchars[256] = {
  1,  87,  49,  12, 176, 178, 102, 166, 121, 193,   6,  84, 249, 230,  44, 163,
 14, 197, 213, 181, 161,  85, 218,  80,  64, 239,  24, 226, 236, 142,  38, 200,
110, 177, 104, 103, 141, 253, 255,  50,  77, 101,  81,  18,  45,  96,  31, 222,
 25, 107, 190,  70,  86, 237, 240,  34,  72, 242,  20, 214, 244, 227, 149, 235,
 97, 234,  57,  22,  60, 250,  82, 175, 208,   5, 127, 199, 111,  62, 135, 248,
174, 169, 211,  58,  66, 154, 106, 195, 245, 171,  17, 187, 182, 179,   0, 243,
132,  56, 148,  75, 128, 133, 158, 100, 130, 126,  91,  13, 153, 246, 216, 219,
119,  68, 223,  78,  83,  88, 201,  99, 122,  11,  92,  32, 136, 114,  52,  10,
138,  30,  48, 183, 156,  35,  61,  26, 143,  74, 251,  94, 129, 162,  63, 152,
170,   7, 115, 167, 241, 206,   3, 150,  55,  59, 151, 220,  90,  53,  23, 131,
125, 173,  15, 238,  79,  95,  89,  16, 105, 137, 225, 224, 217, 160,  37, 123,
118,  73,   2, 157,  46, 116,   9, 145, 134, 228, 207, 212, 202, 215,  69, 229,
 27, 188,  67, 124, 168, 252,  42,   4,  29, 108,  21, 247,  19, 205,  39, 203,
233,  40, 186, 147, 198, 192, 155,  33, 164, 191,  98, 204, 165, 180, 117,  76,
140,  36, 210, 172,  41,  54, 159,   8, 185, 232, 113, 196, 231,  47, 146, 120,
 51,  65,  28, 144, 254, 221,  93, 189, 194, 139, 112,  43,  71, 109, 184, 209
};



/*
**	Calculate hash for a name using randomised table lookup.
*/

static int
hashname(name)
	char *		name;
{
	register int	hash = 0;
	register Uchar *cp = (Uchar *)name;
	register int	c;

	while ( (c = *cp++) )
		hash = randchars[hash ^ c];

#	if	HASHSIZE == 256
	return hash;
#	else	/* HASHSIZE == 256 */
	return hash % HASHSIZE;
#	endif	/* HASHSIZE == 256 */
}



/*
**	Install a symbol in the table.
*/

Symbol *
install(s, t)
	char *			s;	/* symbol to install */
	int 			t;	/* type code for this symbol */
{
	register int		c;
	register char *		cp1;
	register char *		cp2;
	register Symbol **	spp;
	register Symbol *	sp;
	Symbol *		null;

	if ( (c = *s) == 0 )
	{
		/** No need to install null names **/

		Trace1(5, "install ()");
		spp = &null;
		goto out;
	}

	Trace2(4, "install(%s)", s);

	for
	(
		spp = &symtab[hashname(s)] ;
		(sp = *spp) != (Symbol *)0 ;
	)
	{
		Trace2(5, "\"%s\"", sp->name);

		for
		(
			cp1 = s, cp2 = sp->name ;
			(c = *cp1++) != 0 ;
		)
			if ( (c -= *cp2++) )
				break;

		if ( c == 0 )
		{
			if ( (c = *cp2) == 0 )
			{
				Trace2(3, "found %s", s);
				return sp;
			}

			spp = &sp->less;
		}
		else
		if ( c > 0 )
			spp = &sp->great;
		else
			spp = &sp->less;
	}

	Trace2(3, "install %s", s);
out:
	*spp = sp = Talloc0(Symbol);
	sp->name = newstr(s);
	sp->type = t;
	sp->vtype = UNDEF;

	return sp;
}
