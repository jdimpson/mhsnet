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
**	Message data-base.
*/

#define	FILE_CONTROL
#define	LOCKING

#include	"global.h"
#include	"debug.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"msgsdb.h"


static long	MDB_addr;
static long	MDB_align;
static char *	MDB_buffer;
static bool	MDB_flush;
static bool	MDB_open;
static char *	MDB_name;
static bool	MDB_real;

static char *	copy_source _FA_((MDB_Ent *, char *));
static void	write_buffer _FA_((void));



/*
**	Close and unlock data-base.
*/

void
CloseMsgsDB()
{
	Trace1(1, "CloseMsgsDB");

	if ( MDB_open )
	{
		if ( MDB_flush )
			write_buffer();

		FreeStr(&MDB_rname);
		FreeStr(&MDB_buffer);

#		if	AUTO_LOCKING != 1
		(void)UnLock(MDB_rfd);
#		endif	/* AUTO_LOCKING != 1 */

		(void)close(MDB_rfd);
		MDB_open = false;
	}
}



/*
**
*/

static char *
copy_source(e, s)
	MDB_Ent *	e;
	char *		s;
{
	register char *	sp;
	register char *	np;
	register char *	ep;

	Trace2(3, "copy_source(%s)", s);

	np = e->id;

	ep = &np[SOURCE_LENGTH];

	if ( (sp = s) != NULLSTR )
		while ( np < ep )
			if ( (*np++ = *sp++) == '\0' )
			{
				--np;
				break;
			}

	e->len = (np - e->id) + UNIQUE_NAME_LENGTH;

	Trace2(4, "copy_source ==> %d", e->len);

	return np;
}



/*
**	Look up unique id/source in data-base,
**	return true if it already exists,
**	otherwise install new entry with timeout and return false.
*/

bool
LookupMsgsDB(id, source, timeout)
	char *			id;
	char *			source;
	Ulong			timeout;
{
	register int		count;
	long			hash_addr;
	register MDB_Ent *	free_entry;
	register MDB_Ent *	this_entry;
	MDB_Ent			new_entry;

	Trace4(2, "LookupMsgsDB(%s, %s, %s)", id, source, TimeString(timeout, (char *)&new_entry));

	new_entry.timeout = timeout + Time + MARGIN;

	(void)strncpy(copy_source(&new_entry, source), id, UNIQUE_NAME_LENGTH);

	hash_addr = (Ulong)HASH(new_entry.id, new_entry.len) * BUCKET_SIZE;

	Trace4(4, "db_hash{%.*s}==>%#lx", new_entry.len, new_entry.id, (PUlong)(hash_addr/BUCKET_SIZE));

	if ( !ReadMsgsDB(hash_addr) )
		free_entry = MDB_bucket;
	else
	{
		free_entry = (MDB_Ent *)0;

		for ( this_entry = MDB_bucket, count = ENTRY_COUNT ; --count >= 0 ; this_entry++ )
		{
			if
			(
				new_entry.len == this_entry->len
				&&
				strncmp(new_entry.id, this_entry->id, new_entry.len) == STREQUAL
			)
			{
				Trace1(2, "LookupMsgsDB true");
				return true;
			}

			if ( free_entry == (MDB_Ent *)0 && this_entry->timeout < Time )
				free_entry = this_entry;	/* First expired record */
		}
	}

	if ( free_entry != (MDB_Ent *)0 )
	{
		*free_entry = new_entry;
		WriteMsgsDB(hash_addr);
	}
	else
		Warn
		(
			english("Message data-base bucket %ld overflow for id \"%.*s\""),
			hash_addr/BUCKET_SIZE,
			new_entry.len,
			new_entry.id
		);

	return false;
}



/*
**	Rename data-base.
*/

void
NameMsgsDB(name)
	char *	name;
{
	Trace2(1, "NameMsgsDB(%s)", name);

	CloseMsgsDB();

	FreeStr(&MDB_name);

	MDB_name = newstr(name);
}



/*
**	Open and lock data-base.
*/

void
OpenMsgsDB()
{
	register Ulong	bit;

	Trace1(1, "OpenMsgsDB");

	if ( !MDB_open )
	{
		for ( bit = ENTRY_SIZE ; bit ; bit <<= 1 )
			if ( bit == FILEBUFFERSIZE )
				break;

		if ( bit == 0 )
			Error(english("OpenMsgsDB(): FILEBUFFERSIZE (%ld) not power of 2"), (long)FILEBUFFERSIZE);

		if ( bit < BUCKET_SIZE )
			MDB_bsize = BUCKET_SIZE;
		else
			MDB_bsize = bit;
		MDB_align = ~((Ulong)MDB_bsize - 1);

		Trace3(2, "OpenMsgsDB: buffersize = %ld, align = %#lx", MDB_bsize, (PUlong)MDB_align);

		MDB_addr = -1;
		MDB_buffer = Malloc(MDB_bsize);

		if ( MDB_name == NULLSTR )
			MDB_rname = concat(SPOOLDIR, STATEDIR, MSGSDBNAME, NULLSTR);
		else
			MDB_rname = newstr(MDB_name);
		MDB_wname = MDB_rname;

		while ( (MDB_rfd = open(MDB_rname, O_RDWR)) == SYSERROR )
			if ( !MakeLock(MDB_rname) )
				Syserror(CouldNot, LockStr, MDB_rname);

		MDB_wfd = MDB_rfd;

#		if	AUTO_LOCKING != 1
		while ( Lock(MDB_rname, MDB_rfd, for_writing) == SYSERROR )
			Syserror(CouldNot, LockStr, MDB_rname);
#		endif	/* AUTO_LOCKING != 1 */

		Trace3(2, "OpenMsgsDB() \"%s\" ==> %d", MDB_rname, MDB_rfd);

		MDB_reads = 0;
		MDB_writes = 0;

		MDB_flush = false;
		MDB_open = true;
	}
}



/*
**	Read the bucket at address (aligned bufferred reads).
*/

bool
ReadMsgsDB(addr)
	long			addr;
{
	register long		align_addr;
	register MDB_Ent *	entry;
	register int		count;

	if ( !MDB_open )
		OpenMsgsDB();

	Trace2(9, "ReadMsgsDB(%ld)", addr);

	if ( (align_addr = addr & MDB_align) != MDB_addr )
	{
		if ( MDB_flush )
			write_buffer();

		MDB_addr = align_addr;

		for ( ;; )
		{
			while ( lseek(MDB_rfd, align_addr, 0) == SYSERROR )
				Syserror(CouldNot, SeekStr, MDB_rname);

			if ( (count = read(MDB_rfd, MDB_buffer, MDB_bsize)) == SYSERROR )
			{
				Syserror(CouldNot, ReadStr, MDB_rname);
				continue;
			}

			MDB_reads++;

			if ( count == MDB_bsize )
			{
				MDB_real = true;
				break;
			}

			MDB_real = false;

			for
			(
				entry = (MDB_Ent *)MDB_buffer, count = (MDB_bsize/ENTRY_SIZE) ;
				--count >= 0 ;
				entry++
			)
			{
				entry->timeout = 0;
				entry->len = 0;
			}

			break;
		}
	}

	MDB_bucket = (MDB_Ent *)&MDB_buffer[(int)(addr - align_addr)];

	return MDB_real;
}



/*
**	Flush data-base.
*/

void
SyncMsgsDB()
{
	Trace1(3, "SyncMsgsDB");

	if ( MDB_open && MDB_flush )
		write_buffer();
}



/*
**	Write the current buffer.
*/

static void
write_buffer()
{
	Trace1(3, "write_buffer()");

	for ( ;; )
	{
		while ( lseek(MDB_wfd, MDB_addr, 0) == SYSERROR )
			Syserror(CouldNot, SeekStr, MDB_wname);

		if ( write(MDB_wfd, MDB_buffer, MDB_bsize) == MDB_bsize )
			break;

		Syserror(CouldNot, WriteStr, MDB_wname);
	}

	MDB_writes++;
	MDB_flush = false;
}



/*
**	Write the bucket at address.
*/

void
WriteMsgsDB(addr)
	long	addr;
{
	Trace2(3, "WriteMsgsDB(%ld)", addr);
	MDB_flush = true;
}
