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
#define	STAT_CALL

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"header.h"
#include	"spool.h"

#include	<ndir.h>


Ulong		PartsLength;		/* Total size of parts */

static Ulong	CmdsLength;		/* Accumulated data length in parts commands */
static bool	Data;			/* Current parts commands are inside data */
static Ulong	HdrStart;		/* Start address of header in parts commands */
static char *	SpoolDir;		/* Template for spooling commands in MPMSGSDIR */
static char *	LockFile;		/* Template for lock file MPMSGSDIR */
static CmdHead	*TheseCmds;		/* Pointer to CmdHead for current part */

static int	getpartno _FA_((char *, int *, char *));
bool		getmpcommand _FA_((CmdT, CmdV));



/*
**	Search message parts directory for other parts' command files.
**
**	If all present, set up all_cmds with commands for all parts,
**	(sort in part_cmds), and remove directory. Otherwise install
**	part in (new) directory.
*/

bool
AllParts(part_cmds, data_start, data_end, hdr_end, all_cmds, workname, prefix)
	CmdHead *		part_cmds;
	Ulong			data_start;
	Ulong			data_end;
	Ulong			hdr_end;
	CmdHead *		all_cmds;
	char *			workname;
	char *			prefix;
{
	register DIR *		dirp;
	register struct direct *direp;
	register int		partno;
	register char *		bitvec;
	register CmdHead *	partvec;
	register char *		fp;
	register int		thispartno;
	int			maxpart		= MAX_INT;

	PartsLength = data_end - data_start;

	thispartno = getpartno(HdrPart, &maxpart, NULLSTR);

	SpoolDir = concat(
				SPOOLDIR, MPMSGSDIR, DomainToPath(HdrSource),
				Slash, prefix, Slash, HdrID, Slash, NULLSTR
			 );

	/*
	** If // routers - race condition here - so must use lock.
	** (SetLock will create directory if necessary.)
	*/

	partno = 0;
	LockFile = concat(SpoolDir, LOCKFILE, NULLSTR);
	while ( !SetLock(LockFile, Pid, false) )
	{
		if ( ++partno > 3 )
		{
			Syserror(CouldNot, CreateStr, LockFile);
			partno = 1;
		}

		(void)sleep((Pid & 03) + partno);
	}

	(void)getpartno(HdrPart, &maxpart, bitvec = Malloc0((maxpart+7)/8));

	while ( (dirp = opendir(SpoolDir)) == NULL )
		Syserror(CouldNot, OpenStr, SpoolDir);

	while ( (direp = readdir(dirp)) != NULL )
	{
		if ( strchr(direp->d_name, ':') == NULLSTR )
			continue;

		(void)getpartno(direp->d_name, &maxpart, bitvec);
	}

	for ( partno = 0 ; partno < maxpart ; partno++ )
		if ( !(bitvec[partno/8] & (1 << (partno%8))) )
		{
			CmdHead		commands;
			CmdV		file_timeout;
			CmdV		hdr_length;
			CmdV		nak_on_timeout;

			closedir(dirp);
			free(bitvec);

			if ( (file_timeout.cv_number = HdrTtd) == 0 )
				file_timeout.cv_number = WEEK;
			nak_on_timeout.cv_number = NAK_ON_TIMEOUT;
			hdr_length.cv_number = data_end - data_start;	/* Really used as "header start" */

			InitCmds(&commands);
			(void)AddCmd(&commands, hdrlength_cmd, hdr_length);	/* 1st in file */
			LinkCmds(part_cmds, &commands, data_start, hdr_end, workname, Time);
			(void)AddCmd(&commands, timeout_cmd, file_timeout);
			(void)AddCmd(&commands, flags_cmd, nak_on_timeout);

			fp = concat(SpoolDir, HdrPart, NULLSTR);
			partno = create(fp);
			(void)WriteCmds(&commands, partno, fp);
			(void)close(partno);

			free(fp);
			(void)unlink(LockFile);
			free(LockFile);
			free(SpoolDir);

			return false;
		}

	partvec = (CmdHead *)Malloc(maxpart*sizeof(CmdHead));

#	if	BAD_REWINDDIR
	closedir(dirp);
	while ( (dirp = opendir(SpoolDir)) == NULL )
		Syserror(CouldNot, OpenStr, SpoolDir);
#	else	/* BAD_REWINDDIR */
	rewinddir(dirp);
#	endif	/* BAD_REWINDDIR */

	while ( (direp = readdir(dirp)) != NULL )
	{
		if ( strchr(direp->d_name, ':') == NULLSTR )
			continue;

		InitCmds(TheseCmds = &partvec[getpartno(direp->d_name, &maxpart, NULLSTR)]);
		Data = true;
		HdrStart = MAX_ULONG;
		CmdsLength = 0;

		fp = concat(SpoolDir, direp->d_name, NULLSTR);
		if ( !ReadCmds(fp, getmpcommand) )
			Error(CouldNot, english("read commands from"), fp);
		(void)unlink(fp);
		free(fp);
	}

	closedir(dirp);

	(void)RmDir(SpoolDir);

	for ( partno = 0 ; partno < maxpart ; partno++ )
	{
		if ( partno == thispartno )
			LinkCmds(part_cmds, all_cmds, data_start, data_end, workname, Time);
		else
			MoveCmds(&partvec[partno], all_cmds);
	}

	(void)unlink(LockFile);

	free((char *)partvec);
	free(bitvec);

	free(LockFile);
	free(SpoolDir);

	return true;
}



/*
**	Process a command from parts commands file.
*/

bool
getmpcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	static Ulong	filestart;

	switch ( type )
	{
	case end_cmd:
		if ( !Data )
			return true;
		if ( (CmdsLength += val.cv_number - filestart) >= HdrStart )
		{
			val.cv_number -= (CmdsLength - HdrStart);
			Data = false;
		}
		PartsLength += val.cv_number - filestart;
		break;

	case file_cmd:
		if ( !Data )
			return true;
		break;

	case hdrlength_cmd:
		HdrStart = val.cv_number;
		return true;

	case start_cmd:
		if ( !Data )
			return true;
		filestart = val.cv_number;
		break;

	case unlink_cmd:
		break;

	default:
		return true;
	}

	(void)AddCmd(TheseCmds, type, val);

	return true;
}



/*
**	Extract part number from string, compare against max, and set bit in vector.
*/

static int
getpartno(s, maxp, vec)
	char *		s;
	int *		maxp;
	char *		vec;
{
	register char *	cp;
	register int	partno;
	register int	maxno;

	if ( (cp = strchr(s, ':')) == NULLSTR )
		Error(english("Bad message part no. \"%s\""), s);

	*cp++ = '\0';

	if ( (partno = atoi(s)) <= 0 || (maxno = atoi(cp)) > *maxp || maxno <= 0 || partno > maxno )
	{
		*--cp = ':';

		Error
		(
			english("Bad message part name \"%s\" in directory \"%s\", maxparts = %d"),
			s, SpoolDir, *maxp
		);

		return 0;
	}

	*--cp = ':';

	*maxp = maxno;

	partno--;

	if ( vec != NULLSTR )
		vec[partno/8] |= (1 << (partno%8));

	return partno;
}
