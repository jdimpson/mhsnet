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
**	Definitions for Args() argument parsing routine.
*/

/*
**	Argument passed to (*Args.func)((PassVal)value, (Pointer)argp, (char *)string)
*/

typedef union
{
	bool	b;
	char	c;
	double	d;
	float	f;
	long	l;
	char *	p;
	short	s;
}
			PassVal;

typedef char *	AFuncv;
typedef AFuncv	(*AFuncp)_FA_((PassVal, Pointer, char *));

typedef struct
{
	Uchar	type;	/* BOOL, STRING, CHAR, FLOAT, INT, OCT, HEX, NUMBER */
	Uchar	opt;	/* OPTARG, OPTVAL, MANY, ... */
#	if	ANSI_C
	Uchar	flag[2];/* NONFLAG, MINFLAG or "A"... */
#	else	/* ANSI_C */
	Uchar	flag;	/* NONFLAG, MINFLAG or 'A'... */
#	endif	/* ANSI_C */
	Uint	posn;	/* Mandatory position, or OPTPOS */
	Pointer	argp;	/* Where to put the value */
	AFuncp	func;	/* Optional function */
	char *	descr;	/* Optional description for usage message */
	int	len;	/* For ArgsUsage() */
}
			Args;

#if	ANSI_C
#define	flagchar	flag[0]
#else	/* ANSI_C */
#define	flagchar	flag
#endif	/* ANSI_C */

/*
**	Values that can be returned by (*Args.func)()
*/

#define	ACCEPTARG	(AFuncv)0
#define	REJECTARG	(AFuncv)1
#define	ARGERROR	(AFuncv)2
	/* Or an error string */

/*
**	Options for Args.opt
*/

#define	OPTARG		0001	/* Argument is optional */
#define	OPTVAL		0002	/* Value is optional */
#define	MANY		0004	/* Many occurences of the argument */
#define	NFPOS		0010	/* "posn" excludes flags */
#define	ANYFLAG		0020	/* Accept any flag argument */
#define	FOUND		0040	/* Internal evaluation flag */
#define	REJECT		0100	/* Internal evaluation flag */
#define	SPECIAL		0200	/* Special parameter -- means special type */

/*
**	Types for Args.type
*/

#define	BOOL		1	/* Argument of type (int) */
#define	STRING		2	/* Argument of type (char *) */
#define	CHAR		3	/* Argument of type (char) */
#define	INT		4	/* Argument of type (int) */
#define	FLOAT		5	/* Argument of type (float) */
#define	LONG		6	/* Argument of type (long) */
#define	OCT		7	/* Argument of type (long) */
#define	HEX		8	/* Argument of type (long) */
#define	NUMBER		9	/* Any one of INT, OCT, HEX, depending on leading characters */
#define	DOUBLE		10	/* Argument of type (double) */
#define	SHORT		11	/* Argument of type (short) */
#define	MAXTYPE		12

#define	IGNDUPS		(MAXTYPE+0)	/* Don't consider duplicated arguments an error */
#define	IGNNOMATCH	(MAXTYPE+1)	/* Ignore unmatched arguments */

/*
**	Alternatives for Args.posn
*/

#define	OPTPOS		MAX_UINT	/* Position of arg is optional */

/*
**	Alternatives for Args.flag
*/

#define	NONFLAG		'\1'	/* This argument is not a flag */
#define	NULLFLAG	'\2'	/* This argument accepts anything */
#define	MINFLAG		'\0'	/* This argument is just a '-' */
#define	SPECIALFLAGS	'\2'	/* Anything <= this is one of above */

/*
**	Macros for argument specification.
**
**	Arguments are, by default, limited to 1,
**	and, if not booleans, are mandatory, and need a value.
**
**	Variations are specified by including options in last argument.
*/

#if	ANSI_C
#define	Arg_0(P,R)		{STRING, 0, {NONFLAG}, 0, (Pointer)P, (AFuncp)R}
#define	Arg_N(N,P,R,C,T,O)	{T, NFPOS|O, {NONFLAG}, N, (Pointer)P, (AFuncp)R, (char *)C}
#define	Arg_macro(F,P,R,C,T,O)	{T, O, {F}, OPTPOS, (Pointer)P, (AFuncp)R, (char*)C}
#define	Arg_igndups		{IGNDUPS, SPECIAL}
#define	Arg_ignnomatch		{IGNNOMATCH, SPECIAL}
#define	Arg_end			{0}

#define	Arg_any(P,R,C,O)	Arg_macro(NULLFLAG,	P,R,C,	STRING,	O)
#define	Arg_minus(P,R)		Arg_macro(MINFLAG,	P,R,0,	BOOL,	OPTARG)
#define	Arg_noflag(P,R,C,O)	Arg_macro(NONFLAG,	P,R,C,	STRING,	O)

#define	Arg_bool(F,P,R)		Arg_macro(#F,		P,R,0,	BOOL,	OPTARG)
#define	Arg_char(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	CHAR,	O)
#define	Arg_double(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	DOUBLE,	O)
#define	Arg_float(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	FLOAT,	O)
#define	Arg_hex(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	HEX,	O)
#define	Arg_int(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	INT,	O)
#define	Arg_long(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	LONG,	O)
#define	Arg_number(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	NUMBER,	O)
#define	Arg_oct(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	OCT,	O)
#define	Arg_short(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	SHORT,	O)
#define	Arg_string(F,P,R,C,O)	Arg_macro(#F,		P,R,C,	STRING,	O)

#else	/* ANSI_C */

#define	Arg_0(P,R)		{STRING, 0, NONFLAG, 0, (Pointer)P, (AFuncp)R}
#define	Arg_N(N,P,R,C,T,O)	{T, NFPOS|O, NONFLAG, N, (Pointer)P, (AFuncp)R, (char *)C}
#define	Arg_macro(F,P,R,C,T,O)	{T, O, F, OPTPOS, (Pointer)P, (AFuncp)R, (char*)C}
#define	Arg_igndups		{IGNDUPS, SPECIAL}
#define	Arg_ignnomatch		{IGNNOMATCH, SPECIAL}
#define	Arg_end			{0}

#define	Arg_any(P,R,C,O)	Arg_macro(NULLFLAG,	P,R,C,	STRING,	O)
#define	Arg_minus(P,R)		Arg_macro(MINFLAG,	P,R,0,	BOOL,	OPTARG)
#define	Arg_noflag(P,R,C,O)	Arg_macro(NONFLAG,	P,R,C,	STRING,	O)

#define	Arg_bool(F,P,R)		Arg_macro('F',		P,R,0,	BOOL,	OPTARG)
#define	Arg_char(F,P,R,C,O)	Arg_macro('F',		P,R,C,	CHAR,	O)
#define	Arg_double(F,P,R,C,O)	Arg_macro('F',		P,R,C,	DOUBLE,	O)
#define	Arg_float(F,P,R,C,O)	Arg_macro('F',		P,R,C,	FLOAT,	O)
#define	Arg_hex(F,P,R,C,O)	Arg_macro('F',		P,R,C,	HEX,	O)
#define	Arg_int(F,P,R,C,O)	Arg_macro('F',		P,R,C,	INT,	O)
#define	Arg_long(F,P,R,C,O)	Arg_macro('F',		P,R,C,	LONG,	O)
#define	Arg_number(F,P,R,C,O)	Arg_macro('F',		P,R,C,	NUMBER,	O)
#define	Arg_oct(F,P,R,C,O)	Arg_macro('F',		P,R,C,	OCT,	O)
#define	Arg_short(F,P,R,C,O)	Arg_macro('F',		P,R,C,	SHORT,	O)
#define	Arg_string(F,P,R,C,O)	Arg_macro('F',		P,R,C,	STRING,	O)
#endif	/* ANSI_C */

/*
**	Default argument processing.
*/

extern char *		AVersion;
extern int		argerror _FA_((char *));
extern void		DoArgs _FA_((int, char **, Args *));
extern AFuncv		getInt1 _FA_((PassVal, Pointer, char *));
extern AFuncv		getName _FA_((PassVal, Pointer, char *));
extern char *		Name;
extern bool		NoArgsUsage;		/* Set true for no usage message from DoArgs() */
extern void		usagerror _FA_((char *));

/*
**	Argument interpretation.
*/

extern bool		ArgsIgnFlag;		/* All succeeding flags are strings */
extern char *		ArgsUsage _FA_((Args *));
extern char *		ArgTypes[];
extern int		EvalArgs _FA_((int, char **, Args *, int (*)(char *)));
extern bool		ExplainArgs;		/* Set true for full usage message from ArgsUsage() */
extern bool		ExplVersion;		/* Set true for version message from ArgsUsage() */
