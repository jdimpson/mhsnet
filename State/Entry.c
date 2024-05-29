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
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"


/*
**	Pseudorandom permutation of integers 0-255
*/

static Uchar	RandUChars[256] = {
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

static Entry **	List;
static Entry ** Listp;
static bool	Regions;
static char *	REMmap;
bool		StripList;

#if	DEBUG >= 1
static char *	table_name _FA_((Entry **));
#if	DEBUG >= 2
static Entry *	LookupC _FA_((char *, Entry **));
#endif	/* DEBUG >= 2 */
#endif	/* DEBUG >= 1 */
static Index	MLcountTree _FA_((Entry *));
static void	MLfromTree _FA_((Entry *));
int		MLcompare _FA_((const void *, const void *));
static void	REMtree _FA_((Entry *));
static int	HashCName _FA_((char *name));


/*
**	Make an entry in a hash table (ignore case.)
*/

Entry *
Enter(name, table)
	char *			name;
	Entry **		table;
{
	register char		c;
	register char *		cp1;
	register char *		cp2;
	register Entry **	epp;
	register Entry *	ep;

	Trace3(3, "Enter(%s, %s)", name, table_name(table));

#	if	DEBUG == 2
	if ( Traceflag >= 4 )
	{
		Traceflag -= 4;
		if ( Lookup(name, table) == (Entry *)0 )
			Trace(4, "hash \"%s\" => %#x", name, HashName((Uchar *)name));
		Traceflag += 4;
	}
#	endif	/* DEBUG == 2 */

	for
	(
		epp = &table[HashName((Uchar *)name)] ;
		(ep = *epp) != (Entry *)0 ;
	)
	{
#		if	DEBUG == 2
		if ( Traceflag >= 3 )
		{
			Traceflag -= 3;
			if ( Lookup(name, table) == (Entry *)0 )
				Trace(3, "hash dup with \"%s\"", ep->en_name);
			Traceflag += 3;
		}
#		endif	/* DEBUG == 2 */

		for ( cp1 = name, cp2 = ep->en_name ; (c = *cp1++) ; )
			if ( (c ^= *cp2++) && c != 040 )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
			return ep;

		if ( c & 1 )
			epp = &ep->en_great;
		else
			epp = &ep->en_less;
	}

	*epp = ep = Talloc0(Entry);

	ep->en_name = name;

	return ep;
}



/*
**	Make an entry in a hash table.
*/

Entry *
EnterC(name, table)
	char *			name;
	Entry **		table;
{
	register char		c;
	register char *		cp1;
	register char *		cp2;
	register Entry **	epp;
	register Entry *	ep;

	Trace2(3, "EnterC(%s)", name);

#	if	DEBUG == 2
	if ( Traceflag >= 4 )
	{
		Traceflag--;
		if ( LookupC(name, table) == (Entry *)0 )
			Trace(3, "hash \"%s\" => %#x", name, HashCName(name));
		Traceflag++;
	}
#	endif	/* DEBUG == 2 */

	for
	(
		epp = &table[HashCName(name)] ;
		(ep = *epp) != (Entry *)0 ;
	)
	{
#		if	DEBUG == 2
		if ( Traceflag >= 4 )
		{
			Traceflag--;
			if ( LookupC(name, table) == (Entry *)0 )
				Trace(3, "hash dup with \"%s\"", ep->en_name);
			Traceflag++;
		}
#		endif	/* DEBUG == 2 */

		for ( cp1 = name, cp2 = ep->en_name ; (c = *cp1++) ; )
			if ( c -= *cp2++ )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
			return ep;

		if ( c > 0 )
			epp = &ep->en_great;
		else
			epp = &ep->en_less;
	}

	*epp = ep = Talloc0(Entry);

	ep->en_name = name;

	return ep;
}



/*
**	Calculate hash for a node name using randomised table lookup.
*/

static int
HashCName(name)
	char *		name;
{
	register int	hash = 0;
	register Uchar *cp = (Uchar *)name;
	register int	c;

	while ( (c = *cp++) )
		hash = RandUChars[hash ^ c];

#	if	HASH_SIZE == 256
	return hash;
#	else	/* HASH_SIZE == 256 */
	return hash % HASH_SIZE;
#	endif	/* HASH_SIZE == 256 */
}



/*
**	Calculate hash for a node name using randomised table lookup (ignore case).
*/

int
HashName(name)
	Uchar *		name;
{
	register int	hash = 0;
	register Uchar *cp = name;
	register int	c;

	while ( (c = *cp++) )
		hash = RandUChars[hash ^ (c | 040)];

#	if	HASH_SIZE == 256
	return hash;
#	else	/* HASH_SIZE == 256 */
	return hash % HASH_SIZE;
#	endif	/* HASH_SIZE == 256 */
}



/*
**	Look-up an entry in the Region tree (ignore case.)
*/

Entry *
Lookup(name, table)
	char *		name;
	Entry **	table;
{
	register char	c;
	register char *	cp1;
	register char *	cp2;
	register Entry*	ep;
#	if	DEBUG >= 1
	char *		table_desc = table_name(table);
#	endif	/* DEBUG >= 1 */

	for
	(
		ep = table[HashName((Uchar *)name)] ;
		ep != (Entry *)0 ;
	)
	{
		for
		(
			cp1 = name, cp2 = ep->en_name ;
			(c = *cp1++) != 0 ;
		)
			if ( (c ^= *cp2++) && c != 040 )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
		{
			Trace4(
				4,
				"Lookup(%s, %s) ==> %s",
				name,
				table_desc,
				table==RegionHash?ep->REGION->rg_name
					:table_desc==Unknown?"?"
					:ep->MAP
			);

			return ep;
		}

		if ( c & 1 )
			ep = ep->en_great;
		else
			ep = ep->en_less;
	}

	Trace3(4, "Lookup(%s, %s) NOMATCH", name, table_desc);

	return (Entry *)0;
}



#if	DEBUG >= 2
/*
**	Look-up an entry in the Region tree.
*/

static Entry *
LookupC(name, table)
	char *		name;
	Entry **	table;
{
	register char	c;
	register char *	cp1;
	register char *	cp2;
	register Entry*	ep;

	Trace2(4, "LookupC(%s)", name);

	for
	(
		ep = table[HashCName(name)] ;
		ep != (Entry *)0 ;
	)
	{
		for
		(
			cp1 = name, cp2 = ep->en_name ;
			(c = *cp1++) != 0 ;
		)
			if ( (c -= *cp2++) )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
			return ep;

		if ( c > 0 )
			ep = ep->en_great;
		else
			ep = ep->en_less;
	}

	return (Entry *)0;
}
#endif	/* DEBUG >== 2 */



/*
**	Make a sorted list of entries from a hash table.
*/

Index
MakeList(listp, table, oldcount)
	Entry ***		listp;
	Entry **		table;
	Index			oldcount;
{
	register Index		count;
	register Entry **	epp;
	register Entry **	end;
	Index			val;

	if ( table == RegionHash )
		Regions = true;
	else
		Regions = false;

	for ( count = 0, epp = table, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			count += MLcountTree(*epp);

	if ( count == 0 )
		return count;

	if ( (epp = *listp) != (Entry **)0 && oldcount < count )
	{
		free((char *)*listp);
		epp = (Entry **)0;
	}

	if ( epp == (Entry **)0 )
		*listp = epp = (Entry **)Malloc((int)count * sizeof(Entry *));

	List = Listp = epp;

	for ( epp = table ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			MLfromTree(*epp);
	
	DODEBUG(if((Listp-List)!=count)Fatal3("Bad count in MakeList = %d (should be %d)",count,(Listp-List)));
	Trace3(1, "MakeList(%s) ==> %lu", table_name(table), (PUlong)count);

	if ( (val = count) > 1 )
		qsort((char *)List, (int)count, sizeof(Entry *), MLcompare);
	
	if ( Regions )
		for ( epp = Listp ; count ; )
			(*--epp)->REGION->rg_index = --count;

	return val;
}



static Index
MLcountTree(ep)
	register Entry *	ep;
{
	register Index		count;
	register Region *	rp;

	if ( ep->en_great != (Entry *)0 )
		count = MLcountTree(ep->en_great);
	else
		count = 0;

	if ( ep->en_less != (Entry *)0 )
		count += MLcountTree(ep->en_less);

	if
	(
		(rp = ep->REGION) != (Region *)0
		&&
		(!Regions || rp->rg_route[FASTEST] != (Region *)0)
	)
		++count;

	return count;
}



static void
MLfromTree(ep)
	register Entry *	ep;
{
	register Region *	rp;

	if ( ep->en_great != (Entry *)0 )
		MLfromTree(ep->en_great);

	if ( ep->en_less != (Entry *)0 )
		MLfromTree(ep->en_less);

	if ( (rp = ep->REGION) == (Region *)0 )
	{
		Trace2(2, "MLfromTree(%s) NULL", ep->en_name);
		return;
	}

	if ( Regions && rp->rg_route[FASTEST] == (Region *)0 )
	{
		Trace2(2, "MLfromTree(%s) NOROUTE", ep->en_name);
		return;
	}

	if ( StripList )
		(void)StripCopy(ep->en_name, ep->en_name);

	Trace3(2, "MLfromTree(%s) -> %s", ep->en_name, Regions?rp->rg_route[FASTEST]->rg_name:(char *)rp);

	*Listp++ = ep;
}



int
#if	ANSI_C
MLcompare(const void *epp1, const void *epp2)
#else	/* ANSI_C */
MLcompare(epp1, epp2)
	char *	epp1;
	char *	epp2;
#endif	/* ANSI_C */
{
	return strccmp((*(Entry **)epp1)->en_name, (*(Entry **)epp2)->en_name);
}



/*
**	Clear any entries whose MAP matches "map".
*/

void
RemEntMap(map, table)
	char *			map;
	Entry **		table;
{
	register Entry **	epp;
	register Entry **	end;

	Trace3(1, "RemEntMap(%s, %s)", (map==NULLSTR)?NullStr:map, table_name(table));

	REMmap = map;	/* NULLSTR means ALL! */

	for ( epp = table, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			REMtree(*epp);
}



static void
REMtree(ep)
	register Entry *	ep;
{
	if ( ep->en_great != (Entry *)0 )
		REMtree(ep->en_great);

	if ( ep->en_less != (Entry *)0 )
		REMtree(ep->en_less);

	if
	(
		ep->MAP != NULLSTR
		&&
		(
			REMmap == NULLSTR
			||
			strccmp(REMmap, ep->MAP) == STREQUAL
			||
			strccmp(REMmap, ep->en_name) == STREQUAL
		)
	)
	{
		Trace3(2, "REMtree(%s) remove map \"%s\"", ep->en_name, ep->MAP);
		ep->MAP = NULLSTR;
	}
}



#if	DEBUG >= 1
static char *
table_name(table)
	Entry **	table;
{
	return table==RegionHash?"region"
		:table==AliasHash?"alias"
		:table==AdrMapHash?"address"
		:table==RegMapHash?"regmap"
		:table==TypMapHash?"typemap"
		:table==DomMapHash?"dommap"
		:table==FwdMapHash?"forward"
		:Unknown;
}
#endif	/* DEBUG >= 1 */
