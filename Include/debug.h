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
**	Definitions of debugging routines.
**
**	Report:	report on abnormal condition if DEBUG.
**	Fatal:	abort program, print arguments if DEBUG.
**	Trace:	produce output on TraceFd if DEBUG >= 2.
*/

extern void	Fatal _FA_((char *, ...));
extern void	InitTrace();

extern int	Traceflag;
#ifdef	BUFSIZ
extern FILE *	ErrorFd;
extern FILE *	TraceFd;
#endif

#if	DEBUG >= 1

#define	DODEBUG(A)	A

extern void	Report _FA_((char *, ...));

#define	Report1(A)		Report(A)
#define	Report2(A,B)		Report(A,B)
#define	Report3(A,B,C)		Report(A,B,C)
#define	Report4(A,B,C,D)	Report(A,B,C,D)
#define	Report5(A,B,C,D,E)	Report(A,B,C,D,E)
#define	Report6(A,B,C,D,E,F)	Report(A,B,C,D,E,F)
#define	Report7(A,B,C,D,E,F,G)	Report(A,B,C,D,E,F,G)
/**	If more are added, then must change "Lib/Report.c"	**/

#define	Fatal1(A)		Fatal(A)
#define	Fatal2(A,B)		Fatal(A,B)
#define	Fatal3(A,B,C)		Fatal(A,B,C)
#define	Fatal4(A,B,C,D)		Fatal(A,B,C,D)
#define	Fatal5(A,B,C,D,E)	Fatal(A,B,C,D,E)
#define	Fatal6(A,B,C,D,E,F)	Fatal(A,B,C,D,E,F)
#define	Fatal7(A,B,C,D,E,F,G)	Fatal(A,B,C,D,E,F,G)
/**	If more are added, then must change "Lib/Fatal.c"	**/

#if	DEBUG >= 2

extern void	Trace _FA_((int, char *, ...));

#define	Trace1(L,A)		{if(Traceflag>=L)Trace(L,A);}
#define	Trace2(L,A,B)		{if(Traceflag>=L)Trace(L,A,B);}
#define	Trace3(L,A,B,C)		{if(Traceflag>=L)Trace(L,A,B,C);}
#define	Trace4(L,A,B,C,D)	{if(Traceflag>=L)Trace(L,A,B,C,D);}
#define	Trace5(L,A,B,C,D,E)	{if(Traceflag>=L)Trace(L,A,B,C,D,E);}
#define	Trace6(L,A,B,C,D,E,F)	{if(Traceflag>=L)Trace(L,A,B,C,D,E,F);}
#define	Trace7(L,A,B,C,D,E,F,G)	{if(Traceflag>=L)Trace(L,A,B,C,D,E,F,G);}
#define	Trace8(L,A,B,C,D,E,F,G,H)	{if(Traceflag>=L)Trace(L,A,B,C,D,E,F,G,H);}
#define	Trace9(L,A,B,C,D,E,F,G,H,I)	{if(Traceflag>=L)Trace(L,A,B,C,D,E,F,G,H,I);}
#define	Trace10(L,A,B,C,D,E,F,G,H,I,J)	{if(Traceflag>=L)Trace(L,A,B,C,D,E,F,G,H,I,J);}
/**	If more are added, then must change "Lib/Trace.c"	**/

#else	/* DEBUG >= 2 */

#define	Trace1(L,A)
#define	Trace2(L,A,B)
#define	Trace3(L,A,B,C)
#define	Trace4(L,A,B,C,D)
#define	Trace5(L,A,B,C,D,E)
#define	Trace6(L,A,B,C,D,E,F)
#define	Trace7(L,A,B,C,D,E,F,G)
#define	Trace8(L,A,B,C,D,E,F,G,H)
#define	Trace9(L,A,B,C,D,E,F,G,H,I)
#define	Trace10(L,A,B,C,D,E,F,G,H,I,J)

#endif	/* DEBUG >= 2 */

#else	/* DEBUG >= 1 */

#define	DODEBUG(A)

#define	Report1(A)
#define	Report2(A,B)
#define	Report3(A,B,C)
#define	Report4(A,B,C,D)
#define	Report5(A,B,C,D,E)
#define	Report6(A,B,C,D,E,F)
#define	Report7(A,B,C,D,E,F,G)

#define	Fatal1(A)		Fatal(EmptyString)
#define	Fatal2(A,B)		Fatal(EmptyString)
#define	Fatal3(A,B,C)		Fatal(EmptyString)
#define	Fatal4(A,B,C,D)		Fatal(EmptyString)
#define	Fatal5(A,B,C,D,E)	Fatal(EmptyString)
#define	Fatal6(A,B,C,D,E,F)	Fatal(EmptyString)
#define	Fatal7(A,B,C,D,E,F,G)	Fatal(EmptyString)

#define	Trace1(L,A)
#define	Trace2(L,A,B)
#define	Trace3(L,A,B,C)
#define	Trace4(L,A,B,C,D)
#define	Trace5(L,A,B,C,D,E)
#define	Trace6(L,A,B,C,D,E,F)
#define	Trace7(L,A,B,C,D,E,F,G)
#define	Trace8(L,A,B,C,D,E,F,G,H)
#define	Trace9(L,A,B,C,D,E,F,G,H,I)
#define	Trace10(L,A,B,C,D,E,F,G,H,I,J)

#endif	/* DEBUG >= 1 */

#ifndef	MALLOC_DEBUG
#define	MALLOC_DEBUG		0
#endif
