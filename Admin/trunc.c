/*
**	Truncate a file if it exists, preserving modify date.
*/

char		Usage[]		= "Usage: %s file ...\n";
char *		Name;


#include	<stdio.h>
#include	<signal.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>


int		Done;
char *		Nomem		= "out of memory on request for %d bytes";

#define		NULLSTR		(char *)0

void
		catch(),
		error(),
		finish(),
		usage(),
		warn();

extern int	errno;

extern char
		* strrchr();


main(argc, argv)
	int		argc;
	register char *	argv[];
{
	register int	c;

	if ( (Name = strrchr(*argv, '/')) != NULLSTR )
		Name++;
	else
		Name = *argv;

	while ( --argc > 0 )
	{
		if ( **++argv == '-' )
		{
			while ( c = *++*argv )
			{
				register int	n = atoi(&argv[0][1]);

				switch ( c )
				{
				default:
					warn("unrecognised flag '%c'", c);
					usage();
					return 1;
				}
				
				/*while ( isdigit(argv[0][1]) )
					++*argv;*/
			}
		}
		else
		{
			trunc(*argv);
		}
	}

	if ( !Done )
	{
		usage();
		return 1;
	}

	return 0;
}

void
finish()
{
	(void)fflush(stdout);
}

void
_errp(mesg, s, a1, a2)
	char *	mesg;
	char *	s;
	char *	a1;
	char *	a2;
{
	(void)fflush(stdout);
	(void)fprintf(stderr, "%s: %s -- ", Name, mesg);
	(void)fprintf(stderr, s, a1, a2);
}

/*VARARGS1*/
void
warn(s, a1, a2)
	char *	s;
	char *	a1;
	char *	a2;
{
	_errp("warning", s, a1, a2);
	putc('\n', stderr);
}

/*VARARGS2*/
void
error(en, s, a1, a2)
	register int	en;
	char *		s;
	char *		a1;
	char *		a2;
{
	extern char *	sys_errlist[];
	extern int	sys_nerr;

	_errp("error", s, a1, a2);
	if ( en )
		(void)fprintf(stderr, ": %s\n", en<sys_nerr ? sys_errlist[en] : "Unknown error");
	else
		putc('\n', stderr);
	finish();
	exit(2);
}

void
usage()
{
	(void)fprintf(stderr, Usage, Name);
}


int
trunc(fn)
	char *	fn;
{
	register int	fd;
	struct stat 	statb;
	struct
	{
		long int	atime;
		long int	modtime;
	}
			smdate;

	if ( (fd = open(fn, O_RDONLY)) == -1 )
		error(errno, "cannot open file '%s'", fn);

	fstat(fd, &statb);
	close(fd);
	fd = creat(fn, 0);
	smdate.atime = statb.st_atime;
	smdate.modtime = statb.st_mtime;
	utime(fn, &smdate);
	close(fd);
	Done = 1;
}
