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
**	Parameters for debugging
*/

#ifndef	pdp11
#define	PKTHIST		100		/* Number of packets to remember */
#else	/* pdp11 */
#define	PKTHIST		6		/* Number of packets to remember */
#endif	/* pdp11 */
#define	PLOGIN		0		/* Packet received */
#define	PLOGOUT		1		/* Packet transmitted */
#define	PLOGSEND	2		/* Packet actually sent */
#define	PKTLOGLEN	7		/* Bytes to show from logged packet */

#if	DEBUG >= 1

#define	SETDEBUGTIME()	Debug_ticks=ticks()
#define	Logpkt(A,B)	{if(Ptraceflag)Plogpkt(A,B);}
#define	Tracehdrs(A)	{if(Ptraceflag)Ptracehdrs(A);}
#if	PSTATSDESC == 1
#define	Dumphist(A)	{if(Ptraceflag)Pdumphist(A);}
#define	Tracepkt(A,B)	{if(Ptraceflag)Ptracepkt(A,B,0);}
#else
#define	Dumphist(A)	{if(Ptraceflag)Pdumphist("");}
#define	Tracepkt(A,B)	{if(Ptraceflag)Ptracepkt(A,"",0);}
#endif

extern Time_t		Debug_ticks;
extern int		Ptraceflag;
extern void
			Pdumphist(),
			Plogpkt(),
			Ptracehdrs(),
			Ptracepkt();

#else	/* DEBUG */

#define	SETDEBUGTIME()
#define	Logpkt(A,B)
#define	Tracepkt(A,B)
#define	Dumphist(A)
#define	Tracehdrs(A)

#endif	/* DEBUG */
