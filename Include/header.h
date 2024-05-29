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
**	Message header format and memory image.
*/

#define	HDR_TYPE1	'1'		/* Type 1 header */
#define	HDR_TYPE2	'2'		/* Type 2 header */

#define	HDR_LENGTH_SIZE	8		/* Fixed length field at end of message */
#define	MIN_HDR_LEN	16		/* Minimum length for a header + CRC */

#define	ROUTE_RS	'\036'		/* Route separator in route field */
#define	ROUTE_US	'\037'		/* Time separator in route field */

#define	ENV_RS		'\036'		/* Environment record separator */
#define	ENV_US		'\037'		/* Separate environment name from value */

/*
**	Order of fields in header.
*/

typedef enum
{
	ht_type,
	ht_dest, ht_source,
	ht_handler, ht_env,
	ht_ttd, ht_tt, ht_route,
	ht_subpt, ht_id, ht_part,
	ht_quality, ht_flags,
	hr_badread, hr_badcrc, hr_badlen,
#define	ht_n	hr_badread
	hr_ok
}
			HdrReason;

#define	NHDRFIELDS	(int)ht_n

/*
**	Conversion types for header fields.
*/

typedef enum
{
	string_field, number_field, byte_field
}
			HdrConv;

Extern HdrConv		HdrConvs[]
#ifdef	HDR_DATA
=
{
	byte_field,	/* ht_type */
	string_field,	/* ht_dest */
	string_field,	/* ht_source */
	string_field,	/* ht_handler */
	string_field,	/* ht_env */
	number_field,	/* ht_ttd */
	number_field,	/* ht_tt */
	string_field,	/* ht_route */
	byte_field,	/* ht_subpt */
	string_field,	/* ht_id */
	string_field,	/* ht_part */
	byte_field,	/* ht_quality */
	number_field	/* ht_flags */
}
#endif	/* HDR_DATA */
;

/*
**	Vector for header field values.
*/

typedef struct
{
	union
	{
		char *	hf_p;		/* A string */
		Ulong	hf_n;		/* A number */
		char	hf_c;		/* A byte */
	}
		hf_value;
	bool	hf_free;		/* Free pointer if true */
}
			HdrFld;

#define	hf_string	hf_value.hf_p
#define	hf_number	hf_value.hf_n
#define	hf_byte		hf_value.hf_c

typedef struct
{
	int	hdr_length;
	int	hdr_strlength;
	Ulong	hdr_datalength;
	HdrFld	hdr_field[NHDRFIELDS];
}
			HdrFlds;

Extern HdrFlds		HdrFields
#ifdef	HDR_DATA
= { 0 }			/* Some ranlibs can't handle uninitialised data */
#endif	/* HDR_DATA */
;

#define	Hdr_S(A)	HdrFields.hdr_field[(int)A].hf_string
#define	Hdr_N(A)	HdrFields.hdr_field[(int)A].hf_number
#define	Hdr_B(A)	HdrFields.hdr_field[(int)A].hf_byte
#define	HdrDest			Hdr_S(ht_dest)
#define	HdrEnv			Hdr_S(ht_env)
#define	HdrFlags		Hdr_N(ht_flags)
#define	HdrHandler		Hdr_S(ht_handler)
#define	HdrID			Hdr_S(ht_id)
#define	HdrPart			Hdr_S(ht_part)
#define	HdrQuality		Hdr_B(ht_quality)
#define	HdrRoute		Hdr_S(ht_route)
#define	HdrSource		Hdr_S(ht_source)
#define	HdrSubpt		Hdr_B(ht_subpt)
#define	HdrTt			Hdr_N(ht_tt)
#define	HdrTtd			Hdr_N(ht_ttd)
#define	HdrType			Hdr_B(ht_type)

#define	FreeHdr_(A)	HdrFields.hdr_field[(int)A].hf_free
#define	FreeHdrDest		FreeHdr_(ht_dest)
#define	FreeHdrEnv		FreeHdr_(ht_env)
#define	FreeHdrFlags		false
#define	FreeHdrHandler		FreeHdr_(ht_handler)
#define	FreeHdrID		FreeHdr_(ht_id)
#define	FreeHdrPart		FreeHdr_(ht_part)
#define	FreeHdrQuality		false
#define	FreeHdrRoute		FreeHdr_(ht_route)
#define	FreeHdrSource		FreeHdr_(ht_source)
#define	FreeHdrSubpt		false
#define	FreeHdrTt		false
#define	FreeHdrTtd		false
#define	FreeHdrType		false

#define	DataLength	HdrFields.hdr_datalength
#define	HdrLength	HdrFields.hdr_length
#define	HdrStrLength	HdrFields.hdr_strlength

/*
**	Descriptions of fields + errors.
*/

Extern char *		HdrDescs[]
#ifdef	HDR_DATA
=
{
	"TYPE",
	"DESTINATION",
	"SOURCE",
	"HANDLER",
	"ENVIRONMENT",
	"TIME_TO_DIE",
	"TRAVEL_TIME",
	"ROUTE",
	"SUB_PROTOCOL",
	"IDENTIFIER",
	"PART",
	"QUALITY",
	"FLAGS",
	"READ",
	"CRC",
	"LENGTH"
}
#endif	/* HDR_DATA */
;

#define	HeaderReason(A)	HdrDescs[(int)A]

/*
**	Environment field IDs.
*/

#define	ENV_DEST	"DEST"		/* Old HdrDest */
#define	ENV_DEAD	"DEAD"		/* State message specific */
#define	ENV_DOWN	"DOWN"		/* State message specific */
#define	ENV_DUP		"DUP"		/* Message maybe duplicated at this node */
#define	ENV_ERR		"ERR"		/* Error message from router */
#define	ENV_FLAGS	"FLAGS"		/* Used by handlers */
#define	ENV_FLTRFLAGS	"FLTRF"		/* Used by filters */
#define	ENV_FWD		"FWD"		/* Forwarded at this node */
#define	ENV_ID		"ID"		/* HdrID from requesting message */
#define	ENV_NOTREGIONS	"NOTRG"		/* Don't deliver at these nodes */
#define	ENV_ROUTE	"ROUTE"		/* Old HdrRoute */
#define	ENV_SOURCE	"SRC"		/* Old HdrSource */
#define	ENV_TRUNC	"TRUNC"		/* Message was truncated at this node */
#define	ENV_UP		"UP"		/* State message specific */

/*
**	Environment chars that need quoting.
*/

#ifdef	HDR_DATA
char		EnvRestricted[]		= { ENV_RS, ENV_US, '\0' };
#else
extern char	EnvRestricted[];
#endif

/*
**	Header flags -- don't change order.
**	(Bits encoded in number in HdrFlags.)
*/

enum
{
	hf_rqa, hf_ack,
	hf_enq, hf_nak,
	hf_noopt, hf_nocall,
	hf_returned, hf_noret,
	hf_rev_charge,
	hf_registered
};

typedef	Ushort		HFtype;

#ifdef	HDR_FLAGS_DATA
char *	HdrFlagsDescs[] =
{
	"RQA", "ACK",
	"ENQ", "NAK",
	"NOOPT", "NO_AUTOCALL",
	"RETURNED", "NORET",
	"REV_CHARGE",
	"REGISTERED",
	NULLSTR
};
#endif	/* HDR_FLAGS_DATA */

#define	HDR_ACK		(1<<(int)hf_ack)	/* This message is an acknowlegement */
#define	HDR_ENQ		(1<<(int)hf_enq)	/* Reply requested */
#define	HDR_NAK		(1<<(int)hf_nak)	/* Negative acknowlegement */
#define	HDR_NO_AUTOCALL	(1<<(int)hf_nocall)	/* Ignore auto-call flag when spooling */
#define	HDR_NOOPT	(1<<(int)hf_noopt)	/* Don't optimise message order */
#define	HDR_NORET	(1<<(int)hf_noret)	/* Don't return if error */
#define	HDR_REGISTERED	(1<<(int)hf_registered)	/* Message has registered delivery */
#define	HDR_RETURNED	(1<<(int)hf_returned)	/* Message is being returned with error */
#define	HDR_REV_CHARGE	(1<<(int)hf_rev_charge)	/* Reverse charging for this message */
#define	HDR_RQA		(1<<(int)hf_rqa)	/* Request for acknowledgement */

/*
**	External definitions.
*/

extern bool		DelEnv _FA_((char *, char *));
extern bool		ExRevRoute _FA_((char *, char *, char *, bool(*)(char *, Ulong, char *, bool, FILE *), FILE *));
extern bool		ExRoute _FA_((char *, char *, bool(*)(char *, Ulong, char *)));
extern void		FreeHeader _FA_((void));
extern char *		GetEnv _FA_((char *));
extern bool		GetEnvBool _FA_((char *));
extern char *		GetEnvNext _FA_((char *, char *));
extern int		InRoute _FA_((char *, int));
extern char *		MakeEnv _FA_((char *, ...));
extern void		PrintHeader _FA_((FILE *, int, bool));
extern HdrReason	ReadHeader _FA_((int));
extern bool		ReRoutable _FA_((char *));
extern bool		UpdateHeader _FA_((Ulong, char *));
extern int		WriteHeader _FA_((int, Ulong, int));
