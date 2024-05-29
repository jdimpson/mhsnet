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
**	Type definitions for C programming.
*/

typedef unsigned char	Uchar;
typedef unsigned int	Uint;
typedef unsigned short	Ushort;
typedef unsigned long	PUlong;		/* For printf %lu */
#if LONG_LONG
typedef unsigned long long PUllong;	/* For printf %llu */
#else	/* LONG_LONG */
typedef unsigned long	PUllong;	/* For printf %llu */
#endif	/* LONG_LONG */

	/*
	** Try to avoid 64-bit longs.
	*/

#if	BITS_ULONG != 32
#if	BITS_UINT == 32
#undef	BITS_ULONG
#define	BITS_ULONG	32
#undef	MAX_LONG
#define	MAX_LONG	MAX_INT
#undef	MAX_ULONG
#define	MAX_ULONG	MAX_UINT
typedef unsigned int	Ulong;
#else	/* BITS_UINT == 32 */
#undef	ULONG_SIZE
#define	ULONG_SIZE	22
#undef	NUMERICARGLEN
#define	NUMERICARGLEN	(ULONG_SIZE+2)
typedef unsigned long	Ulong;
#endif	/* BITS_UINT == 32 */
#else	/* BITS_ULONG != 32 */
typedef unsigned long	Ulong;
#endif	/* BITS_ULONG != 32 */

#if	LONG_LONG
typedef	long long	Llong;
typedef	unsigned long long	Ullong;
#else	/* LONG_LONG */
#define	Llong		long
#define	Ullong		Ulong
#endif	/* LONG_LONG */

#ifndef	ENUM_NOT_INT
typedef enum { false = 0, true = 1 } bool;
typedef enum { for_reading = 0, for_writing = 1 } Lock_t;
#else	/* ENUM_NOT_INT */
typedef int		bool;
#define	false		0
#define	true		1
typedef int		Lock_t;
#define	for_reading	0
#define	for_writing	1
#endif	/* ENUM_NOT_INT */

typedef	Ushort		Crc_t;
typedef	Ulong		Crc32_t;
typedef int		(*Funcp)();
typedef time_t		Time_t;

#ifdef	NO_VOID
#define	void		int
#undef	NO_VOID_STAR
#define	NO_VOID_STAR
#endif	/* NO_VOID */

#ifdef	NO_VOID_STAR
#ifdef	cray
typedef int *		Pointer;
#else
typedef char *		Pointer;
#endif
typedef int		(*vFuncp)();
typedef	int		(SigFunc)();
#else	/* NO_VOID_STAR */
typedef void *		Pointer;
typedef void		(*vFuncp)();
#ifdef	ANSI_C
typedef	void		(SigFunc)(int);
#else	/* ANSI_C */
typedef	void		(SigFunc)();
#endif	/* ANSI_C */
#endif	/* NO_VOID_STAR */
