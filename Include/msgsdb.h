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
**	Messages data-base definitions.
**
**	The data-base is a (sparse) file of MDB_Ents.
**
**	Hash function is 16-bit CRC.
**
**	At least one MDB_Ent per BUCKET_SIZE buffer
**	==> max data-base file size of HASH_SIZE * BUCKET_SIZE.
**
**	Primary index to an MDB_Ent is HASH * BUCKET_SIZE,
**	followed by a linear search of subsequent contiguous MDB_Ents.
*/

#define	BUCKET_SIZE	128	/* (power of 2) */	/* 8000 active entries ==> 1Mb file (max 4Mb) */
#define	ENTRY_COUNT	(BUCKET_SIZE/ENTRY_SIZE)	/* Entries per bucket */
#define	ENTRY_SIZE	32	/* (power of 2) */	/* Allows short source address */
#define	HASH_MASK	0x7fff				/* 32K */
#define	HASH_SIZE	(HASH_MASK+1)			/* 32K */
#define	HASH(A,B)	(acrc((Crc_t)0,A,B)&HASH_MASK)	/* CRC -> 16 bit hash */
#define	MARGIN		HOUR				/* Allow for stretch */
#define	MSGSDBNAME	"mesgsfile"			/* File name for data-base */

#define	SOURCE_LENGTH	(ENTRY_SIZE-sizeof(Time_t)-sizeof(short)-UNIQUE_NAME_LENGTH)

typedef struct
{
	Time_t	timeout;				/* Absolute timeout (==0 means empty) */
	short	len;					/* Length of ``id'' */
	char	id[SOURCE_LENGTH+UNIQUE_NAME_LENGTH];
}
		MDB_Ent;

/*
**	Externals
*/

Extern int	MDB_bsize;				/* Size of buffers */
Extern MDB_Ent *MDB_bucket;				/* Current bucket */
Extern int	MDB_rfd;				/* Data-base read file descriptor */
Extern int	MDB_wfd;				/* Data-base write file descriptor */
Extern char *	MDB_rname;				/* Data-base read file name */
Extern char *	MDB_wname;				/* Data-base write file name */
Extern Uint	MDB_reads;				/* Count of buffers read */
Extern Uint	MDB_writes;				/* Count of buffers written */

extern void	CloseMsgsDB _FA_((void));
extern bool	LookupMsgsDB _FA_((char *, char *, Ulong));
extern void	NameMsgsDB _FA_((char *));
extern void	OpenMsgsDB _FA_((void));
extern bool	ReadMsgsDB _FA_((long));
extern void	SyncMsgsDB _FA_((void));
extern void	WriteMsgsDB _FA_((long));
