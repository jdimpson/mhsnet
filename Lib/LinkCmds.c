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


#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"spool.h"


vFuncp		NoLinkFunc;

static bool	try_make_link _FA_((char *, char *));


/*
**	While input data in range obase->ofend,
**		for each file in cmdp1,
**		make a link and add commands to cmdp2,
**		using ``newfile'' as a template.
**
**	Call (*NoLinkFunc)(file) if any link could not be made.
*/

void
LinkCmds(cmdp1, cmdp2, obase, ofend, newfile, msgtime)
	CmdHead *	cmdp1;
	CmdHead *	cmdp2;
	Ulong		obase;
	Ulong		ofend;
	char *		newfile;
	Time_t		msgtime;
{
	register Cmd *	cep;
	register Ulong	oposn;
	register CmdV	ibase;
	register CmdV	ifend;
	register CmdV	ifile;
	register Ulong	ilength;
	bool		and_unlink;

	Trace6(1, "LinkCmds(%#lx, %#lx, %lu, %lu, %s)", (PUlong)cmdp1, (PUlong)cmdp2, (PUlong)obase, (PUlong)ofend, newfile);

	if ( ofend <= obase )
		return;

	for ( oposn = 0, cep = cmdp1->ch_first ; cep != (Cmd *)0 && oposn < ofend ; cep = cep->cd_next )
	{
		switch ( cep->cd_type )
		{
		default:
			continue;

		case file_cmd:
			ifile.cv_name = cep->cd_value;
			continue;

		case start_cmd:
			ibase.cv_number = cep->cd_number;
			continue;

		case end_cmd:
			ifend.cv_number = cep->cd_number;
		}

		ilength = ifend.cv_number - ibase.cv_number;

		if ( (oposn + ilength) > obase )
		{
			if ( obase > oposn )
			{
				ibase.cv_number += obase - oposn;
				ilength -= obase - oposn;
				oposn = obase;
			}

			if ( (oposn + ilength) > ofend )
			{
				ifend.cv_number -= (oposn + ilength) - ofend;
				ilength = ofend - oposn;
			}

			Trace4(2, "link file \"%s\" %lu->%lu", ifile.cv_name, (PUlong)ibase.cv_number, (PUlong)ifend.cv_number);

			(void)UniqueName(newfile, CHOOSE_QUALITY, OPTNAME, ilength, msgtime);

			if ( try_make_link(ifile.cv_name, newfile) )
			{
				ifile.cv_name = newfile;
				and_unlink = true;
			}
			else
			{
				if ( NoLinkFunc != NULLVFUNCP )
					(*NoLinkFunc)(ifile.cv_name);
				and_unlink = false;
			}

			(void)AddCmd(cmdp2, file_cmd, ifile);
			(void)AddCmd(cmdp2, start_cmd, ibase);
			(void)AddCmd(cmdp2, end_cmd, ifend);

			if ( and_unlink )
				(void)AddCmd(cmdp2, unlink_cmd, ifile);
		}

		oposn += ilength;
	}
}



/*
**	Attempt to link a file.
*/

static bool
try_make_link(name1, name2)
	char *	name1;
	char *	name2;
{
	Trace3(2, "try_make_link(%s, %s)", name1, name2);

	while ( link(name1, name2) == SYSERROR )
		if ( !CheckDirs(name2) )
		{
			Trace1(3, "try_make_link() FAILED");
			return false;
		}

	return true;
}
