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
**	Formats and magic numbers for licencing.
*/

#define	LICENCE_CHECK_LENGTH	8	/* Length of A-P string from 32-bit CRC */

#define	LICENCE_CHAR_DATE	'*'	/* Licence date indicator */
#define	LICENCE_CHAR_LINK	'@'	/* Licence link indicator */

#define	LICENCE_OK		00000
#define	LICENCE_INCORRECT	00377	/* Any value 00001->00377 */
#define	LICENCE_BAD		00400
#define	LICENCE_EXPIRED		01000
#define	LICENCE_LINK		02000

/*
**	Function declarations.
*/

extern char *	MakeLicence _FA_((char *, char *, char *, int));

#if	CHECK_LICENCE
extern int	CheckLicence _FA_((char *, char *, int, char *));
#else	/* CHECK_LICENCE */
#define		CheckLicence(A,B,C,D)	0
#endif	/* CHECK_LICENCE */
