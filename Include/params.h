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
**	Define structure to select operating parameters.
*/

typedef struct
{
	char *	id;	/* Name of parameter */
	char **	val;	/* Where to put its value */
	short	type;	/* Type (below) */
	short	set;	/* Non-zero if modified */
}
	ParTb;

#define	PSTRING	0	/* Parameter is a string */
#define	PVECTOR	1	/* Parameter is a vector of strings */
#define	PLONG	2	/* Parameter is a long integer */
#define	PINT	3	/* Parameter is an integer */
#define	PDIR	4	/* Parameter is a string needing a '/' appended */
#define PSPOOL	5	/* Parameter is a string needing SPOOLDIR pre-pended */

/**	Table of ParTb must be sorted on ``id'' **/

/*
**	Externals.
*/

extern bool	CheckParams;
extern void	GetParams _FA_((char *, ParTb *, int));
extern ParTb	NetParams[];
extern int	SizeNetParams;
