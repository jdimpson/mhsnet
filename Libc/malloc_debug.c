/*
**	Supply dummy routines for malloc debugging if not available.
**
**	SCCSID @(#)malloc_debug.c	1.2 05/08/26
*/

#ifndef	__APPLE__

int
malloc_debug(level)
	int	level;
{
	return level;
}

#else	/* __APPLE__ */
dummy_malloc_debug(){}
#endif	/* __APPLE__ */

int
malloc_verify()
{
	return 1;	/* OK */
}
