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
**	External identifiers used by ExpandArgs().
*/

Extern bool	Ack;			/* Message has HDR_ACK flag */
Extern bool	Broadcast;		/* Destination address is broadcast */
Extern char *	DataName;		/* Name of FTP data */
Extern Ulong	DataSize;		/* Size of data part of message */
Extern char *	DupNode;		/* Message may have been duplicated there */
Extern char *	EnvError;		/* Error string from header ENV_ERR */
Extern char *	ExHomeAddress;		/* Home address */
Extern char *	ExLinkAddress;		/* Link address */
Extern char *	ExSourceAddress;	/* Source address */
Extern bool	ReqAck;			/* Message has HDR_RQA flag */
Extern bool	Returned;		/* Message has HDR_RETURNED flag */
Extern char *	SenderName;		/* Name of FTP sender */
Extern Time_t	StartTime;		/* Message insertion time */
Extern char *	TruncNode;		/* Message was truncated there */
Extern char *	UserName;		/* Name of FTP receiver */
