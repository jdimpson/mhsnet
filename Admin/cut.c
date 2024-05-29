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


static char	sccsid[]	= "@(#)cut.c	1.9 05/12/16";

/*
**	`cut' -- cut columns or fields from lines.
*/

#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters.
*/

#define		LAST_COL	-1			/* Matche last column */

/*
**	Parameters set from arguments.
*/

char		DelimChar;				/* Delimiter character */
char *		Delims	= " \t";			/* Delimiter string */
char *		Name	= sccsid;			/* Program invoked name */
bool		NoSqueeze;				/* Don't squeeze delimiters */
bool		NoNlSqueeze;				/* Don't squeeze new-lines */
int		Traceflag;				/* Trace control */

AFuncv	getCols _FA_((PassVal, Pointer, char *));	/* Get column list */
AFuncv	getFields _FA_((PassVal, Pointer, char *));	/* Get field list */
AFuncv	getFile _FA_((PassVal, Pointer, char *));	/* Get file list */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(n, &NoNlSqueeze, 0),
	Arg_bool(s, &NoSqueeze, 0),
	Arg_string(c, 0, getCols, english("column-list"), OPTARG|MANY),
	Arg_string(d, &Delims, 0, english("delimiters"), OPTARG),
	Arg_string(f, 0, getFields, english("field-list"), OPTARG|MANY),
	Arg_char(t, &DelimChar, 0, english("delimiter"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(0, getFile, english("file"), OPTARG|MANY),
	Arg_end
};

/*
**	Selections.
*/

typedef struct Range	Range;
struct Range
{
	Range *	next;
	int	start;
	int	stop;
};

int	RangeCount;
Range *	Selections;
bool	SelectChars;
bool	SelectFields;

/*
**	Files.
*/

typedef struct File	File;
struct File
{
	File *	next;
	char *	name;
};

File *	FirstFile;
File **	LastFile = &FirstFile;

/*
**	Miscellaneous
*/

bool	LastCol;

/*
**	Externals
*/

void	cut _FA_((FILE *));
AFuncv	getrange _FA_((char *));
int	listcmp _FA_((char *, char *));
bool	select_col _FA_((int));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register File *	fp;
	register FILE *	fd;

	InitParams();

	DoArgs(argc, argv, Usage);

	if ( SelectChars && SelectFields )
		Error(english("-c and -f are exclusive"));

	if ( DelimChar != '\0' )
	{
		Delims[0] = DelimChar;
		Delims[1] = '\0';
	}

	if ( RangeCount > 1 )
		listsort((char **)&Selections, listcmp);

	setbuf(stdout, SObuf);

	if ( (fp = FirstFile) == (File *)0 )
		cut(stdin);
	else
	do
	{
		while ( (fd = fopen(fp->name, "r")) == NULL )
			Syserror(CouldNot, OpenStr, fp->name);
		cut(fd);
		fclose(fd);
	}
	while
		( (fp = fp->next) != (File *)0 );

	finish(EX_OK);
	return 0;
}



void
cut(fd)
	FILE *		fd;
{
	register int	c;
	register char *	cp;
	register int	col = 0;
	register bool	delim = false;
	register bool	first = true;
	char		buf[512];

	cp = buf;

	while ( (c = getc(fd)) != EOF )
	{
		if ( c == '\n' )
		{
			if ( LastCol && cp > buf )
			{
				if ( SelectChars )
					putc(*--cp, stdout);
				else
					fwrite(buf, cp-buf, 1, stdout);
			}
			if ( !first || NoNlSqueeze )
				putc(c, stdout);
			if ( ferror(stdout) )
			{
				Syserror(CouldNot, WriteStr, "stdout");
				clearerr(stdout);
			}
			col = 0;
			delim = false;
			first = true;
			cp = buf;
			continue;
		}

		if ( SelectFields )
		{
			if ( strchr(Delims, c) != NULLSTR )
			{
				if ( delim )
				{
					if ( NoSqueeze )
						col++;
					else
						continue;
				}
				else
				{
					col++;
					if ( !NoSqueeze )
						c = Delims[0];
				}
				delim = true;
				cp = buf;
				if ( first )
					continue;
			}
			else
				delim = false;
		}

		if ( select_col(col) )
		{
			putc(c, stdout);
			first = false;
			cp = buf;
		}
		else
			*cp++ = c;

		if ( SelectChars )
			col++;
	}
}



void
finish(error)
	int	error;
{
	(void)fflush(stdout);

	exit(error);
}



/*ARGSUSED*/
AFuncv
getCols(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	SelectChars = true;
	return getrange(val.p);
}



/*ARGSUSED*/
AFuncv
getFields(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	SelectFields = true;
	return getrange(val.p);
}



/*ARGSUSED*/
AFuncv
getFile(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register File *	newfile;

	newfile = Talloc(File);
	newfile->name = val.p;
	*LastFile = newfile;
	LastFile = &newfile->next;
	newfile->next = (File *)0;

	return ACCEPTARG;
}



AFuncv
getrange(s)
	char *		s;
{
	register char *	cp;
	register Range *newrange;
	register int	n1;
	register int	n2;
	AFuncv		ret;

	if ( (cp = strchr(s, ',')) != NULLSTR )
	{
		*cp++ = '\0';
		if ( (ret = getrange(cp)) != ACCEPTARG )
			return ret;
	}

	if ( (cp = strchr(s, '-')) != NULLSTR )
	{
		*cp++ = '\0';
		n1 = atol(s);
		if ( strcmp(cp, "$") == STREQUAL )
			n2 = MAX_INT;
		else
			n2 = atol(cp);

		if ( n1 > n2 )
			return (AFuncv)english("-ve range");
	}
	else
	if ( strcmp(s, "$") == STREQUAL )
	{
		LastCol = true;
		n2 = n1 = MAX_INT;
	}
	else
		n2 = n1 = atol(s);

	newrange = Talloc(Range);
	newrange->start = n1;
	newrange->stop = n2;

	newrange->next = Selections;
	Selections = newrange;

	RangeCount++;

	return ACCEPTARG;
}



int
listcmp(p1, p2)
	register char *	p1;
	register char *	p2;
{
	static char	err[] = english("Ranges overlap");

	Trace5(4, "listcmp(%d,%d<>%d,%d)", ((Range *)p1)->start, ((Range *)p1)->stop, ((Range *)p2)->start, ((Range *)p2)->stop);

	if ( ((Range *)p1)->start > ((Range *)p2)->start )
	{
		if ( ((Range *)p1)->start < ((Range *)p2)->stop )
		{
			Error(err);
			return 0;
		}

		return 1;
	}

	if ( ((Range *)p1)->start < ((Range *)p2)->start )
	{
		if ( ((Range *)p1)->stop > ((Range *)p2)->start )
		{
			Error(err);
			return 0;
		}

		return -1;
	}

	Error(english("Ranges start at identical places"));
	return 0;
}



bool
select_col(col)
	register int	col;
{
	register Range *rp;

	for ( rp = Selections ; rp != (Range *)0 ; rp = rp->next )
	{
		Trace4(3, "select_col(%d) <> %d,%d", col, rp->start, rp->stop);
		if ( col < rp->start )
			break;
		if ( col == rp->start || col <= rp->stop  )
		{
			Trace2(2, "select_col(%d) ==> TRUE", col);
			return true;
		}
	}

	Trace2(2, "select_col(%d) ==> FALSE", col);
	return false;
}
