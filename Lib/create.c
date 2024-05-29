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


#define	FILE_CONTROL

#include	"global.h"
#include	"debug.h"



/*
**	Create a file for writing.
*/

int
create(file)
	char *	file;
{
	int	fd;

	Trace2(2, "create(%s)", file);

#	ifdef	O_CREAT
	while ( (fd = open(file, O_CREAT|O_EXCL|O_WRITE, 0600)) == SYSERROR )
#	else	/* O_CREAT */
	while ( (fd = creat(file, 0600)) == SYSERROR )
#	endif	/* O_CREAT */
		if ( !CheckDirs(file) )
			Syserror(CouldNot, CreateStr, file);

	return fd;
}



/*
**	Create a file for writing and reading, and ignore errors.
*/

int
createn(file)
	char *	file;
{
	int	fd;

	Trace2(2, "createn(%s)", file);

#	ifdef	O_CREAT
	while ( (fd = open(file, O_CREAT|O_EXCL|O_RDWR, 0600)) == SYSERROR )
#	else	/* O_CREAT */
	while ( (fd = creat(file, 0600)) == SYSERROR )
#	endif	/* O_CREAT */
		if ( !CheckDirs(file) )
			if ( !SysWarn(CouldNot, CreateStr, file) )
				break;

	return fd;
}



/*
**	Create a file for writing and reading.
*/

int
creater(file)
	char *	file;
{
	int	fd;

	Trace2(2, "creater(%s)", file);

#	ifdef	O_CREAT

	while ( (fd = open(file, O_CREAT|O_EXCL|O_RDWR, 0600)) == SYSERROR )
		if ( !CheckDirs(file) )
			Syserror(CouldNot, CreateStr, file);

#	else	/* O_CREAT */

	while
	(
		close(create(file)) == SYSERROR
		||
		(fd = open(file, O_RDWR)) == SYSERROR
	)
		Syserror(CouldNot, CreateStr, file);

#	endif	/* O_CREAT */

	return fd;
}



/*
**	Link a file.
*/

void
make_link(name1, name2)
	char *	name1;
	char *	name2;
{
	Trace3(2, "make_link(%s, %s)", name1, name2);

	while ( link(name1, name2) == SYSERROR )
		if ( !CheckDirs(name2) )
			Syserror("Can't link \"%s\" to \"%s\"", name1, name2);
}



/*
**	Move a file.
*/

void
move(name1, name2)
	char *	name1;
	char *	name2;
{
	Trace3(2, "move(%s, %s)", name1, name2);

#	if	RENAME_2 == 1 && NFS == 0

	while ( rename(name1, name2) == SYSERROR )
		if ( !CheckDirs(name2) )
			Syserror(english("Can't rename \"%s\" to \"%s\""), name1, name2);

#	else	/* RENAME_2 == 1 && NFS == 0 */

	(void)unlink(name2);
	make_link(name1, name2);
	(void)unlink(name1);

#	endif	/* RENAME_2 == 1 && NFS == 0 */
}
