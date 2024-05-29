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
**	Definition for format of files in FORWARDINGDIR.
*/

typedef struct
{
	char *	handler;			/* Name of handler */
	char *	address;			/* Forwarding address */
	char *	sub_addr;			/* Sub-protocol address */
}
			Forward;		/* Per line in file */

Extern Forward *	Forwds;			/* Set by GetForward() */
Extern int		ForwdCount;
Extern char *		ForwdData;
Extern char *		ForwdFile;

/*
**	Routines.
*/

extern Forward *Forwarded _FA_((char *, char *, char *));	/* Check if entity should be forwarded */
extern Forward *GetForward _FA_((char *, char *));		/* Find forwarding details for entity */
extern void	SetForward _FA_((char *, char *, char *, char *));	/* Set line in forwarding file */
