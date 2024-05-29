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
**	Common parameters for call and shell.
*/

#define	CALLPROG	"callprog"	/* Name of program to be run by VCcall before VCdeamon */
#define	SHELLPROG	"shellprog"	/* Name of program to be run by VCshell before VCdeamon */

#define	BYTESTREAM	0
#define	DATAGRAM	1

/*
**	Default daemon parameters (if failed protocol negotiation.)
**
**	[Cooked/batch mode, data size 16, max run time 5 mins, min speed 10 b/s, throughput 30 b/s.]
*/

#define	DFLTPARAMS	"-cB5 -D16 -R300 -S10 -X30"

/*
**	Setup communication strings.
*/

#define	ALREADYACTIVE	"DAEMON ALREADY ACTIVE\n"
#define	BADPASSWORD	"FAILED PASSWORD\n"
#define	DAEMONSTARTS	"DAEMON STARTS DAEMON STARTS\n"
#define	DAEMONIS	"DAEMON "
#define	DEFAULTSTARTS	"DEFAULT STARTS DEFAULT STARTS\n"
#define	DISALLOWED	"CONNECTION DISALLOWED\n"
#define	HOMENAMEIS	"HOMENAME "
#define	INACTIVE	"NETWORK INACTIVE\n"
#define	PARAMS		"PARAMS "
#define	PASSWORDIS	"PASSWORD "
#define	QUERYDAEMON	"QUERY DAEMON\n"
#define	QUERYHOME	"QUERY HOMENAME\n"
#define	QUERYPARAMS	"QUERY PARAMS\n"
#define	QUERYPASSWORD	"QUERY PASSWORD\n"
#define	SHELLERROR	"SHELL ERROR\n"
#define	SHELLSTARTS	"SHELL STARTS 2V\n"
#define	VCPARAMS	"VCCONF "
#define	XSHELLSTARTS	"XON_XOFF SHELL STARTS 2V\n"

#define	PARAMSTIMEOUT	20

extern char *	StrCRC16 _FA_((char *));
