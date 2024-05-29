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
**	Define file transfer protocol header and memory image.
*/

#define	FTH_TYP2	2	/* Type 2 header */

#define	FTH_LENGTH_SIZE	8	/* Fixed length field at end of header */
#define	MIN_FTH_LENGTH	11	/* Minimum number of bytes in a legal header */
#define	MODE_SIZE	4	/* Number of bytes in a file mode */

#define	FTH_USEP	','	/* Byte to separate user names */
#define	FTH_UDEST	'@'	/* Byte to separate destinations from users */
#define	FTH_UDSEP	';'	/* Byte to separate user/destination pairs */
#define	FTH_FDSEP	','	/* Byte to separate file descriptor fields */
#define	FTH_FSEP	';'	/* Byte to separate file descriptors */

/*
**	Define positions of null terminated fields.
*/

typedef enum
{
	fth_type,
	fth_to, fth_from,
	fth_files,
	fth_badread, fth_badcrc, fth_badlen, fth_baddatacrc,
#define	fth_n	fth_badread
	fth_ok
}
			FthReason;

#define	NFTHFIELDS	(int)fth_n

/*
**	Descriptions of fields.
*/

Extern char *		FthDescs[]
#ifdef	HDR_DATA
=
{
	"TYPE",
	"TO",
	"FROM",
	"DESC",
	"READ",
	"CRC",
	"LENGTH",
	"DATA_CRC"
}
#endif	/* HDR_DATA */
;

#define	FTHREASON(A)	FthDescs[(int)A]

/*
**	Structure for fields.
*/

typedef struct
{
	int	fth_length;
	int	fth_strlength;
	Ulong	fth_datalength;
	char *	fth_start[NFTHFIELDS];
	char	fth_ctype[2];
}
			Fth_Fields;

Extern Fth_Fields	FthFields;

#ifndef	unreasonable
#define	FthType		FthFields.fth_ctype
#define	FthTo		FthFields.fth_start[(int)fth_to]
#define	FthFrom		FthFields.fth_start[(int)fth_from]
#define	FthFdescs	FthFields.fth_start[(int)fth_files]
#else	/* unreasonable */
#define	FthType		FthFields.fth_ctype
#define	FthTo		FthFields.fth_start[1]
#define	FthFrom		FthFields.fth_start[2]
#define	FthFdescs	FthFields.fth_start[3]
#endif	/* unreasonable */

#define	FtDataLength	FthFields.fth_datalength
#define	FthLength	FthFields.fth_length
#define	FthStrLength	FthFields.fth_strlength

/*
**	Type field.
*/

#define	FTH_BASE_TYPE	060		/* Base for 1st byte in type field */
#define	FTH_BASE_MASK	077		/* Range for type */
#define	FTH_DATACRC	0100		/* Data 16-bit CRC present */
#define	FTH_DATA32CRC	0200		/* Data 32-bit CRC present */
#define	FTH_TYPE	(FTH_TYP2+FTH_BASE_TYPE)

#define	TYPEOF_FTH(C)	((C)&FTH_BASE_MASK)

/*
**	Structure of file descriptor list.
*/

typedef struct FthFDesc *FthFD_p;

typedef struct FthFDesc
{
	FthFD_p	f_next;
	char *	f_name;		/* File name */
	char *	f_alias;	/* Transmitted name */
	Ulong	f_length;	/* Length */
	Time_t	f_time;		/* Modify time */
	short	f_mode;		/* Permissions */
}
			FthFDesc;

#define	FTH_MODES	00777	/* Permission bits from inode */
#define	FTH_NOT_IN_MESG	01000	/* Flag used for truncated messages */
#define	FTH_STDIN	02000	/* Flag used for "stdin" file */

/*
**	Structure of user list.
*/

typedef struct FthUlist	*FthUl_p;

typedef struct FthUlist
{
	FthUl_p	u_next;
	char *	u_name;		/* A user */
	char *	u_dest;		/* Associated address */
	char *	u_dir;		/* Associated directory */
	int	u_uid;		/* Associated ``uid'' */
}
			FthUlist;

/*
**	Name/address chars that need quoting.
*/

#ifdef	HDR_DATA
char			FthFdRestricted[]	= { FTH_FSEP, FTH_FDSEP, '\0' };
char			FthToRestricted[]	= { FTH_USEP, FTH_UDEST, FTH_UDSEP, '\0' };
#else
extern char		FthFdRestricted[];
extern char		FthToRestricted[];
#endif

/*
**	External definitions.
*/

Extern Crc_t		DataCrc;
Extern Crc32_t		Data32Crc;
Extern char *		FtHeader;
Extern FthFDesc *	FthFiles;
Extern FthFD_p		FthFFreeList;
Extern FthUlist *	FthUsers;
Extern FthUlist *	FthUFreeList;
Extern int		NFthFiles;
Extern int		NFthUsers;
Extern char *		UQFthTo;

extern void		FreeFthFiles _FA_((void));
extern void		FthToFree _FA_((void));
extern FthReason	GetFthFiles _FA_((void));
extern int		GetFthTo _FA_((bool));
extern bool		InFthTo _FA_((char *));
extern void		InitFtHeader _FA_((void));
extern FthReason	ReadFtHeader _FA_((int, Ulong));
extern int		SetFthFiles _FA_((void));
extern void		SetFthTo _FA_((void));
extern int		WriteFtHeader _FA_((int, Ulong));
extern void		PrintFtHeader _FA_((FILE *, int, bool));
