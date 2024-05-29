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
**	Manipulate status files.
*/

#define	STAT_CALL
#define	FILE_CONTROL

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"


/*
**	Data buffers.
*/

static char	DataBuffers[NCHANNELS][FILEBUFFERSIZE];

/*
**	Version of statusfile should be:
*/

static char	ChanVersion[]	= CHANVERSION;



/*
**	Read in any old status file and fix up data, or initialise new status.
*/

int
InitStatus(out)
	register bool	out;
{
	register Chan *	chnp;
	register int	i;
	register int	shortlen;
	register int	wholelen;
	register char *	filep;

	Status_File = out ? WRITERSTATUS : READERSTATUS;

	Trace2(1, "InitStatus \"%s\"", Status_File);

	i = MIN_STATUS_SIZE(Status);

	if
	(
		(StatusFd = open(Status_File, O_RDWR)) == SYSERROR
		||
		Read(StatusFd, (char *)&Status, i, Status_File) != i
		||
		strcmp(Status.st_version, ChanVersion) != STREQUAL
	)
	{
		if ( StatusFd != SYSERROR )
		{
			Report1(english("status file version mis-match"));
			Close(StatusFd, CLOSE_NOSYNC, Status_File);
			(void)strclr((char *)&Status, i);
		}

		(void)unlink(Status_File);
		StatusFd = Create(Status_File);
		(void)chmod(Status_File, 0644);
	}

	Status.st_pid = Pid;
	Status.st_pktdatasize = OutDataSize;
	(void)strcpy(Status.st_version, ChanVersion);

	Status.st_flags = CHLINKDOWN|CHLINKSTART;
	if ( CookVC != (CookT *)0 )
	{
		Status.st_flags |= CHCOOKEDVC;
		i = CookVC - Cookers;
		Status.st_flags |= (i & 0xf) << 8;
	}
	if ( UseDataCRC )
		Status.st_flags |= (OutDataSize>MAXCRCSIZE)?CHDATACRC32:CHDATACRC16;
	if ( TurnAround )
		Status.st_flags |= CHTURNAROUND;
	if ( InOnly )
		Status.st_flags |= CHINONLY;
	if ( OutOnly )
		Status.st_flags |= CHOUTONLY;
#	if	VCDAEMON_STATS == 1
	Status.st_flags |= CHDMNSTATS;
	(void)strclr((char *)DmnStats, sizeof(DmnStats));
#	endif
#	if	PROTO_STATS == 1
	Status.st_flags |= CHPROSTATS;
	(void)strclr((char *)PktStats, sizeof(PktStats));
#	endif

	StartTime = CTime = Time;	/* Initialise times kept in Status */

	DecayTime = Time;
	DecayActive();			/* Set next decay time */
	ActiveTime = 80 * ACT_MULT;	/* Effectively 60 seconds grace */

	if ( (ActiveBytes = ((NomSpeed * OutDataSize) / MaxPktSize(OutDataSize)) * 80 * ACT_MULT) == 0 )
		if ( (ActiveBytes = MinSpeed * 2 * 80 * ACT_MULT) == 0 )
			ActiveBytes = 240 * 80 * ACT_MULT;	/* Default nominal speed */

	if ( (ActiveRawBytes = NomSpeed * 80 * ACT_MULT) == 0 )
		ActiveRawBytes = ActiveBytes;

	DecayCount = 0;
	PacketTime = 0;

	filep = concat(SPOOLDIR, LinkName, Slash, out?CmdsName:MsgsInName, Slash, NULLSTR);
	shortlen = strlen(filep);
	wholelen = shortlen + MSGIDLENGTH + 1;

	for ( chnp = Channels, i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		chnp->ch_number = i;

		if ( out )
		{
			chnp->ch_flags = 0;
			chnp->ch_partlist = (Part *)0;
			chnp->ch_current = (Part *)0;
			chnp->ch_cntrltime = 0;

			chnp->ch_cmdfilename = strcpy(Malloc(wholelen), filep);
			chnp->ch_cmdfilename[wholelen-1] = '\0';
			chnp->ch_msgidp = &chnp->ch_cmdfilename[shortlen];

/*			(void)strclr((char *)chnp->ch_statsout, sizeof(WCHstat));
*/			chnp->ch_statsout(wch_datanak) = 0;
		}
		else
		{
			if ( chnp->ch_state == CH_ACTIVE || chnp->ch_state == CH_ENDING )
			{
				chnp->ch_flags &= CH_RDUP;
				chnp->ch_state = CH_STARTING;
			}
			else
				chnp->ch_flags = 0;

			chnp->ch_gaplist = (Gap *)0;
			chnp->ch_eomcount = 0;
			chnp->ch_msgaddr = chnp->ch_goodaddr;

			chnp->ch_msgfilename = strcpy(Malloc(wholelen), filep);
			chnp->ch_msgfilename[wholelen-1] = '\0';
			chnp->ch_msgidp = &chnp->ch_msgfilename[shortlen];

/*			(void)strclr((char *)chnp->ch_statsin, sizeof(RCHstat));
*/			chnp->ch_statsin(rch_datanak) = 0;
		}

		chnp->ch_msgid[MSGIDLENGTH] = '\0';
		(void)strcpy(chnp->ch_msgidp, chnp->ch_msgid);

		chnp->ch_databuffer = DataBuffers[i];
		chnp->ch_bufstart = 0;
		chnp->ch_bufend = 0;

		if ( out )
		{
			if
			(
				chnp->ch_msgid[0] > '\0'
				&&
				SetupParts(chnp)
			)
			{
				Report7(
					RepFmt,
					i,
					ChnStateStr(chnp),
					english("OUT restarted"),
					chnp->ch_msgid,
					(PUlong)chnp->ch_msgsize,
					(PUlong)chnp->ch_msgaddr
				);

				(void)StartMessage(chnp, false);
			}
			else
			{
				chnp->ch_state = CH_IDLE;
				chnp->ch_msgid[0] = '\0';
				chnp->ch_msgaddr = 0;
			}
		}
	}

	free(filep);

	StatusTime = Time;
	Write_Status();

	return 0;
}



/*
**	Write out current status.
*/

void
Write_Status()
{
	register Ulong	avg;
	Ulong		change_time;
	static Ulong	last_data;
	static Time_t	last_time;

	DODEBUG(char	tbuf1[DATETIMESTRLEN]);
	DODEBUG(char	tbuf2[DATETIMESTRLEN]);

	if ( StatusTime == 0 )
		return;

	Trace4(
		1,
		"Write_Status \"%s\", Time= %s, StatusTime= %s",
		Status_File,
		DateTimeStr(Time, tbuf1)+9,
		DateTimeStr(StatusTime, tbuf2)+9
	);

	if ( (change_time = Time - last_time) != 0 )
	{
		Status.st_datarate = (Status.st_dmnstats[DS_DATA] - last_data) / change_time;
		last_data = Status.st_dmnstats[DS_DATA];
		last_time = Time;
	}

	(void)Seek(StatusFd, (long)0, 0, Status_File);

	Write(StatusFd, (char *)&Status, sizeof(VCstatus), Status_File);

	StatusTime = Time + UpdateTime;

	if ( (avg = Status.st_datarate) > 0 && change_time < 10 )
		if ( (avg = Status.st_act_time) > 0 )
			avg = Status.st_act_bytes / avg;

	NameProg("%s %s %lu %lu", Name, SLinkAddress, (PUlong)Status.st_dmnstats[DS_MESSAGES], (PUlong)avg);
}
