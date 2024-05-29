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
**	Clear route details, if set.
*/

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"



void
FreeRoute()
{
	if ( RouteBase == NULLSTR )
		return;

	free(RouteBase);
	RouteBase = NULLSTR;

	MaxTypes = 0;
	TypeCount = 0;
	LinkCount = 0;
	TypeEntries = 0;
	RegionCount = 0;
	AdrMapCount = 0;
	RegMapCount = 0;
	TypMapCount = 0;
	MaxLinkLength = 0;
	MaxStrLength = 0;

	TypeNameVector = (Index *)0;
	TypeVector = (TypeTab *)0;
	TypeTables = (TypeEnt *)0;
	LinkVector = (RLink *)0;
	RegionVector = (RegionEnt *)0;
	NameVector = (MapEnt *)0;
	AdrMapVector = (MapEnt *)0;
	RegMapVector = (MapEnt *)0;
	TypMapVector = (MapEnt *)0;
	RegionTable = NULLSTR;
	Strings = NULLSTR;
	HomeName = NULLSTR;
	VisibleName = NULLSTR;
#	if	CHECK_LICENCE
	LicenceNumber = NULLSTR;
#	endif	/* CHECK_LICENCE */
}
