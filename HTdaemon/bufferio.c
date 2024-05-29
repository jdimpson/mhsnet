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
**	Message file data bufferring.
*/

#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"chnstats.h"
#include	"channel.h"



/*
**	Flush read buffer, reset pointers.
*/

void
FlushRData(chnp, addr)
	register Chan *	chnp;
	Ulong		addr;
{
	Trace4(
		2, "FlushRData %lu bytes at address %lu on channel %d",
		(PUlong)(chnp->ch_bufend - chnp->ch_bufstart),
		(PUlong)addr, chnp->ch_number
	);

	chnp->ch_msgaddr = addr;
}



/*
**	Flush write buffer, reset pointers.
*/

void
FlushWData(chnp)
	register Chan *	chnp;
{
	register Ulong	size;

	Trace2(2, "FlushWData on channel %d", chnp->ch_number);

	if ( (size = chnp->ch_bufend - chnp->ch_bufstart) != 0 )
	{
		Trace4(3,
			"FlushWData %#lx->%#lx file \"%s\"",
			(PUlong)chnp->ch_bufstart,
			(PUlong)chnp->ch_bufend,
			chnp->ch_msgfilename
		);
		Write(chnp->ch_msgfd, chnp->ch_databuffer, (int)size, chnp->ch_msgfilename);
	}

	chnp->ch_bufstart = chnp->ch_bufend;
}



/*
**	Produce small packets by aligned bigger read from a message.
*/

int
ReadData(chnp, data, size)
	register Chan *	chnp;
	char *		data;
	register int	size;
{
	register Part *	pp;
	register Ulong	readsize;
	register Ulong	seekaddr;
	register Ulong	offset;

	Trace6(
		3, "ReadData size %d on channel %d message \"%s\" addr %lu size %lu",
		size, chnp->ch_number, chnp->ch_msgid,
		(PUlong)chnp->ch_msgaddr, (PUlong)chnp->ch_msgsize
	);

	if ( chnp->ch_msgaddr < chnp->ch_bufstart || chnp->ch_msgaddr >= chnp->ch_bufend )
	{
		Trace3(4, "bufstart %lu bufend %lu", (PUlong)chnp->ch_bufstart, (PUlong)chnp->ch_bufend);

		if
		(
			(pp = chnp->ch_current) == (Part *)0
			||
			chnp->ch_msgaddr < pp->msgstart_part
		)
			pp = chnp->ch_partlist;

		for ( readsize = 0 ; pp != (Part *)0 ; pp = pp->next_part )
		{
			if ( chnp->ch_msgaddr >= pp->msgend_part )
				continue;

			readsize = pp->msgend_part - chnp->ch_msgaddr;

			Trace3(4, "readsize %lu msgend_part %lu", (PUlong)readsize, (PUlong)pp->msgend_part);

			break;
		}

		if ( readsize == 0 )
			return 0;

		if ( chnp->ch_current != pp || !(chnp->ch_flags & CH_MSGFILE) )
		{
			if ( chnp->ch_flags & CH_MSGFILE )
			{
				Close(chnp->ch_msgfd, CLOSE_NOSYNC, chnp->ch_current->filename_part);
				chnp->ch_flags &= ~CH_MSGFILE;
			}

			chnp->ch_current = pp;
			pp->fileaddr_part = 0;

			chnp->ch_msgfd = OpenJ(chnp->ch_number, pp->filename_part);

			chnp->ch_flags |= CH_MSGFILE;
		}

		seekaddr = pp->filestart_part + (chnp->ch_msgaddr - pp->msgstart_part);

		if ( size > (FILEBUFFERSIZE/2) || (chnp->ch_flags & CH_IGNBUF) )
		{
			if ( seekaddr != pp->fileaddr_part )
				(void)SeekJ(chnp->ch_number, chnp->ch_msgfd, seekaddr, 0, pp->filename_part);

			if ( size > readsize )
				size = readsize;

			Trace4(
				3, "ReadData %d bytes from file \"%s\" at %lu",
				size, pp->filename_part, (PUlong)seekaddr
			);

			size = ReadJ(chnp->ch_number, chnp->ch_msgfd, data, size, pp->filename_part);

			pp->fileaddr_part = seekaddr + size;
			goto out;
		}

		if ( (offset = seekaddr & FILEBUFFERMASK) > 0 )
		{
			Trace2(3, "ReadData offset %lu", (PUlong)offset);
			seekaddr &= ~FILEBUFFERMASK;
			readsize += offset;
		}

		if ( seekaddr != pp->fileaddr_part )
		{
			(void)SeekJ(chnp->ch_number, chnp->ch_msgfd, seekaddr, 0, pp->filename_part);
			pp->fileaddr_part = seekaddr;
		}

		if ( readsize > FILEBUFFERSIZE )
			readsize = FILEBUFFERSIZE;

		Trace4(
			3, "ReadData %lu bytes from file \"%s\" at %lu",
			(PUlong)readsize, pp->filename_part, (PUlong)seekaddr
		);

		if
		(
			(readsize = ReadJ
				    (
					chnp->ch_number,
					chnp->ch_msgfd,
					chnp->ch_databuffer,
					(int)readsize,
					pp->filename_part
				    )
			) == 0
		)
			return 0;

		pp->fileaddr_part += readsize;

		chnp->ch_bufstart = chnp->ch_msgaddr - offset;
		chnp->ch_bufend = chnp->ch_bufstart + readsize;
	}

	if ( size > (readsize = chnp->ch_bufend - chnp->ch_msgaddr) )
		size = readsize;

	bcopy(&chnp->ch_databuffer[chnp->ch_msgaddr - chnp->ch_bufstart], data, size);

	Trace2(3, "ReadData buffer %d bytes", size);
out:
	if ( (chnp->ch_msgaddr += size) == chnp->ch_msgsize )
		chnp->ch_flags |= CH_EOF;

	return size;
}



/*
**	Save up small packets for aligned bigger writes to a file.
**
**	(``ch_bufstart'' represents write point in file.)
*/

void
WriteData(chnp, data, size, addr)
	register Chan *	chnp;
	char *		data;
	register int	size;
	register Ulong	addr;
{
	register Ulong	bufend;
	register Ulong	left;

	Trace4(4, "WriteData chan %d bytes %d addr %lu", chnp->ch_number, size, (PUlong)addr);

	if ( size > (FILEBUFFERSIZE/2) && addr >= chnp->ch_bufend )
	{
		if ( chnp->ch_bufend > chnp->ch_bufstart )
			FlushWData(chnp);

		if ( chnp->ch_bufstart != addr )
			(void)Seek(chnp->ch_msgfd, addr, 0, chnp->ch_msgfilename);

		Trace4(3, "WriteData %#lx->%#lx file \"%s\"", (PUlong)addr, (PUlong)(addr+size), chnp->ch_msgfilename);
		Write(chnp->ch_msgfd, data, size, chnp->ch_msgfilename);

		chnp->ch_bufstart = chnp->ch_bufend = addr + size;
		return;
	}

	if ( addr < chnp->ch_bufstart )
	{
		(void)Seek(chnp->ch_msgfd, addr, 0, chnp->ch_msgfilename);

		if ( (addr + size) <= chnp->ch_bufstart )
		{
			Trace4(3, "WriteData %#lx->%#lx file \"%s\"", (PUlong)addr, (PUlong)(addr+size), chnp->ch_msgfilename);
			Write(chnp->ch_msgfd, data, size, chnp->ch_msgfilename);

			if ( chnp->ch_bufstart != (addr + size) )
				(void)Seek(chnp->ch_msgfd, chnp->ch_bufstart, 0, chnp->ch_msgfilename);
			return;
		}

		left = chnp->ch_bufstart - addr;

		Trace4(3, "WriteData %#lx->%#lx file \"%s\"", (PUlong)addr, (PUlong)(addr+left), chnp->ch_msgfilename);
		Write(chnp->ch_msgfd, data, (int)left, chnp->ch_msgfilename);

		size -= left;
		data += left;
		addr += left;
	}

	if ( addr > chnp->ch_bufend )	/* Can't allow gaps in buffer */
	{
		if ( chnp->ch_bufend > chnp->ch_bufstart )
			FlushWData(chnp);

		(void)Seek(chnp->ch_msgfd, addr, 0, chnp->ch_msgfilename);
		chnp->ch_bufend = chnp->ch_bufstart = addr;
	}

	while ( (addr + size) > (bufend = (chnp->ch_bufstart & ~FILEBUFFERMASK) + FILEBUFFERSIZE) )
	{
		if ( addr < bufend )
		{
			left = bufend - addr;

			Trace4(3, "WriteData buffer %lu bytes %#lx->%#lx", (PUlong)left, (PUlong)addr, (PUlong)(addr+left));
			bcopy(data, &chnp->ch_databuffer[addr - chnp->ch_bufstart], (int)left);

			size -= left;
			data += left;
			addr += left;
			chnp->ch_bufend = bufend;
		}

		if ( chnp->ch_bufend > chnp->ch_bufstart )
			FlushWData(chnp);

		if ( (chnp->ch_bufstart = addr) != chnp->ch_bufend )
		{
			(void)Seek(chnp->ch_msgfd, chnp->ch_bufstart, 0, chnp->ch_msgfilename);
			chnp->ch_bufend = chnp->ch_bufstart;
		}
	}

	Trace4(3, "WriteData buffer %d bytes %#lx->%#lx", size, (PUlong)addr, (PUlong)(addr+size));
	bcopy(data, &chnp->ch_databuffer[addr - chnp->ch_bufstart], size);

	if ( (bufend = addr + size) > chnp->ch_bufend )
		chnp->ch_bufend = bufend;
}
