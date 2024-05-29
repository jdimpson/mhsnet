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
**	Reasons for "BadMessage" in "children.c".
*/

typedef enum
{
	bm_nodata, bm_readdata, bm_pastEOF, bm_size, bm_read, bm_name, bm_format, bm_badlink
}
	BMReason;

#ifdef	BM_DATA
char *	BMExplanations[] =
{
	english("Can't open data file"),
	english("Can't read data file"),
	english("Tried to read past EOF"),
	english("Bad command file size"),
	english("Can't read command file"),
	english("Bad name for command file"),
	english("Bad format for commands"),
	english("Bad link name in command file")
};
#endif	/* BM_DATA */

extern void	BadMessage _FA_((AQarg, BMReason));
