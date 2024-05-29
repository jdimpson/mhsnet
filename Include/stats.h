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


#define	STATSFILE	"Accumulated"	/* In STATSDIR */
#define	CONNECTLOG	"connect"	/* In STATSDIR */

/*
**	Format of message statistics file:
**
**	<new-line><ID><record> ...
*/

#define	ST_SEP		'\n'	/* Record separator */
#define	ST_RE_SEP	':'	/* Record element separator */

/*
**	Possible 'ID's.
*/

#define	ST_F_S_LOG	'F'	/* Record is from fileserver log */
#define	ST_INMESG	'I'	/* Record is from inbound message */
#define	ST_OUTMESG	'O'	/* Record is from outbound message */

/*
**	Routing statistics record
*/

typedef enum
{
	sm_time, sm_id, sm_flags, sm_handler, sm_tt, sm_size, sm_source, sm_dest, sm_link, sm_delay
#define	sm_last	sm_delay
}
	StMesg_t;

/*
**	Fileserver statistics record
*/

typedef enum
{
	fl_time, fl_id, fl_size, fl_dest, fl_service, fl_names
#define	fl_last	fl_names
}
	FSlog_t;

/*
**	Routines
*/

extern bool	CloseStats _FA_((void));
extern bool	FlushStats _FA_((void));
extern bool	WrStats _FA_((int, bool (*)()));
