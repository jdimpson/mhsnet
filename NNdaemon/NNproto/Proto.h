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
**	Definition of a control packet's first data byte
*/

#define	CONTROL		0200		/* Identify PAD control */

#define	TIMEOUT		(CONTROL|0)	/* Are you there ? */
#define	XMT_RESET	(CONTROL|1)	/* Please reset transmitter and reply */
#define	ACK_RESET	(CONTROL|2)	/* Transmitter reset acknowledge and reset receiver */
#define	REQ_RESET	(CONTROL|3)	/* Please send an XMT_RESET */
#define	IDLE		(CONTROL|4)	/* Dum-di-dum */

#ifdef	PNproto
#define	TIMO_SIZ	4
#else
#define	TIMO_SIZ	1
#endif
#if	defined(NNstrpr) || defined(NNstrpr2)
#define	XMT_R_SIZ	3
#endif
#if	defined(ENproto) || defined(PNproto)
#define	XMT_R_SIZ	4
#endif
#define	ACK_R_SIZ	1
#define	REQ_R_SIZ	1
#define	IDLE_SIZ	1

#define	MAX_CTL_SIZ	XMT_R_SIZ

/** User definable control packets have CONTROL bit off **/
