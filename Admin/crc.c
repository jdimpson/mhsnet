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


static char	sccsid[]	= "@(#)crc.c	1.7 05/12/16";

/*
**	crc - print decimal crc32 of file(s).
*/

#define	FILE_CONTROL
#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"Args.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

char *	Name		= sccsid;	/* Program invoked name */
int	Traceflag;

/*
**	Arguments.
*/

AFuncv	getAcn _FA_((PassVal, Pointer, char *));
AFuncv	getBin _FA_((PassVal, Pointer, char *));
AFuncv	getOct _FA_((PassVal, Pointer, char *));
AFuncv	getHex _FA_((PassVal, Pointer, char *));
AFuncv	getFile _FA_((PassVal, Pointer, char *));
AFuncv	getFileM _FA_((PassVal, Pointer, char *));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, 0, getAcn),		/* Output in ascii coded number */
	Arg_bool(b, 0, getBin),		/* Output in binary */
	Arg_bool(o, 0, getOct),		/* Output in octal */
	Arg_bool(x, 0, getHex),		/* Output in hexadecimal */
	Arg_minus(0, getFileM),		/* Read "stdin" */
	Arg_noflag(0, getFile, "file", OPTARG|MANY),
	Arg_end
};

/*
**	Miscellaneous
*/

char	Buf[33];		/* Buffer for conversion */
Crc32_t	FilesCRC;		/* The accumulated CRC32 of files */
char *	(*Ffuncp) _FA_((Ulong));/* Conversion function */
bool	FileRead;		/* Processed a file */
char *	Format	= "%lu\n";	/* Output format for FilesCRC */
#define	FormatC	PUlong

char *	getbinary _FA_((Ulong));
char *	getcodenum _FA_((Ulong));
AFuncv	getcrc _FA_((int, char *));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	DoArgs(argc, argv, Usage);

	if ( !FileRead )
	{
		PassVal		nullv;

		(void)getFileM(nullv, (Pointer)0, NULLSTR);
	}

	if ( Ffuncp != (char *(*)())0 )
		printf(Format, (*Ffuncp)((Ulong)FilesCRC));
	else
		printf(Format, (FormatC)FilesCRC);

	exit(EX_OK);
	return 0;
}



void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Process file.
*/

AFuncv
getFile(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	int	fd;

	while ( (fd = open(value.p, O_READ)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, value.p) )
			return ARGERROR;

	return getcrc(fd, value.p);
}



AFuncv
getFileM(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	return getcrc(fileno(stdin), "stdin");
}



AFuncv
getcrc(fd, file)
	register int	fd;
	char *		file;
{
	register int	r;
	char		buf[FILEBUFFERSIZE];

	FileRead = true;

	for ( ;; )
	{
		while ( (r = read(fd, buf, FILEBUFFERSIZE)) <= 0 )
			if ( r == 0 || !SysWarn(CouldNot, ReadStr, file) )
			{
				(void)close(fd);
				return ACCEPTARG;
			}

		FilesCRC = acrc32(FilesCRC, buf, r);
	}
}



/*
**	Set different output formats.
*/

AFuncv
getAcn(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	Ffuncp = getcodenum;
	Format = "%s\n";
	return ACCEPTARG;
}

AFuncv
getBin(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	Ffuncp = getbinary;
	Format = "%s\n";
	return ACCEPTARG;
}

AFuncv
getHex(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	Format = "%lx\n";
	return ACCEPTARG;
}

AFuncv
getOct(value, argp, string)
	PassVal	value;
	Pointer	argp;
	char *	string;
{
	Format = "%lo\n";
	return ACCEPTARG;
}



/*
**	Return string with binary representaion of long.
*/

char *
getb(cp, n)
	char *	cp;
	Ulong	n;
{
	if ( n & 0xfffffffe )
		cp = getb(cp, n>>1);

	*cp++ = '0' + (n&1);

	return cp;
}



char *
getbinary(l)
	Ulong		l;
{
	(void)getb(Buf, l);

	return Buf;
}



char *
getcodenum(l)
	Ulong		l;
{
	(void)EncodeNum(Buf, l, -1);

	return Buf;
}
