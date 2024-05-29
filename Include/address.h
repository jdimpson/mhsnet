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


#define	DOMAIN_SEP	'.'	/* Char. to separate domains of a destination */
#define	TYPE_SEP	'='	/* Char. to separate type from name in a domain */

/*
**	Address types:
**
**	The following leading characters in a destination address string
**	identify the address type.
*/

#define	ATYP_BROADCAST		'*'
#define	ATYP_DESTINATION	'.'
#define	ATYP_EXPLICIT		'!'
#define	ATYP_MULTICAST		','

/*
**	Addresses may take the following forms:-
**
**	address		::=	multicast|explicit|broadcast|destination
**	multicast	::=	<,>{explicit|broadcast|destination}...
**	explicit	::=	<!>{broadcast|destination}...
**	broadcast	::=	<*>destination
**	destination	::=	<.>domain[...]
**	domain		::=	type<=>name
**	type | name	::=	<a|A>..<z|Z><0>..<9><-><_>< >
*/

/*
**	Structure for containing parts of an address.
*/

typedef struct Address	Addr;
struct Address
{
	Addr *	ad_down;	/* Must be first */
	Addr *	ad_up;
	Addr *	ad_next;
	Addr *	ad_prev;
	char *	ad_typed;	/* If != 0, should be free'd */
	char *	ad_untyped;	/* If != 0, should be free'd */
	char *	ad_string;	/* If != 0, should be free'd */
	char	ad_type[2];	/* ATYP in [0] */
	short	ad_flags;	/* AD_ flags - see below */
};

#define	AD_CLONED		0x1	/* Don't free dest */
#define	AD_FORWARDED		0x2	/* Don't follow explicit */

#define	ADDRTYPE(p)		((p)->ad_type[0])
#define	BCSTTYPE(p)		(ADDRTYPE(p)==ATYP_BROADCAST)
#define	DESTTYPE(p)		(ADDRTYPE(p)==ATYP_DESTINATION)
#define	EXPLTYPE(p)		(ADDRTYPE(p)==ATYP_EXPLICIT)
#define	MLTITYPE(p)		(ADDRTYPE(p)==ATYP_MULTICAST)

#define	ADDRDEST(p)		((Dest *)((p)->ad_down))
#define	ADDRDESTP(p)		((Dest **)(&(p)->ad_down))
#define	SET_ADDRDEST(p,B)	(p)->ad_down = (Addr *)(B)

/*
**	Type filling.
*/

typedef enum
{
	notype, intype, maptype
}
		ExpType;

/*
**	Name macros for debugging only.
*/

#if	DEBUG >= 2
#define	DESTNAME(p)		(((p)==(Dest*)0||(p)->dt_name==NULLSTR)?NullStr:(p)->dt_name)
#define	ADDRNAME(p)		((p)==(Addr*)0?NullStr:DESTTYPE(p)?DESTNAME(ADDRDEST(p)):(p)->ad_type)
#endif	/* DEBUG >= 2 */

/*
**	Structure for containing domain names of a destination.
*/

#define	MAXPARTS	10	/* Max. number of domains normally encountered */

typedef struct
{
	int	dn_type;	/* Canonical type */
	char *	dn_tname;	/* Start of type */
	char *	dn_name;	/* Start of name */
}
			Dname;

typedef struct
{
	Dname *	dt_names;	/* Pointer to array of domains */
	Dname	dt_parts[MAXPARTS]; /* Array of domains if count small */
	char *	dt_name;	/* Untyped name, should be free'd */
	char *	dt_tname;	/* Typed name, should be free'd */
	char *	dt_string;	/* If != 0, should be free'd */
	int	dt_count;	/* Count of domains */
}
			Dest;

#define	NEXTDEST(p)		((Dest *)((p)->dt_names))	/* For freelist */
#define	SET_NEXTDEST(p,B)	(p)->dt_names = (Dname *)(B)

/*
**	Third parameter for AddrAtHome.
*/

#define	AAH_FOLLOW	true	/* Follow explicit route and test last */
#define	NO_AAH_FOLLOW	false	/* Disallow explicit route following */

/*
**	Second parameter for StringToAddr.
*/

#define	STA_MAP		false	/* Allow translation for `forwarded' addresses */
#define	NO_STA_MAP	true	/* Disallow address forwarding */

/*
**	Externals
*/

extern char *		MatchedBC;	/* Broadcast address matched by AddressMatch() */
extern bool		MatchRegion;	/* Allow match within region in AddressMatch() */

#ifdef	ANSI_C

#ifndef	P_BROADCAST

#ifdef	ExternU
#undef	Extern
#define	Extern	extern		/* Don't declare any data accidentally */
#endif	/* ExternU */

#include	"Passwd.h"

#ifdef	ExternU
#undef	Extern
#define	Extern
#endif	/* ExternU */

#endif	/* !P_BROADCAST */

typedef void	(*CA_ep)(char *, ...);

#else	/* ANSI_C */

#define	CA_ep	vFuncp

#endif	/* ANSI_C */

extern void		AddAddr _FA_((Addr **, Addr *));
extern bool		AddrAtHome _FA_((Addr **, bool, bool));
extern Addr *		AddrFromUser _FA_((char *));
extern bool		AddrIsBC _FA_((Addr *));
extern char *		AddrToString _FA_((Addr *, ExpType));
extern char *		CanonString _FA_((char *));
extern char *		CheckAddr _FA_((Addr **, Passwd *, CA_ep, bool));
extern Addr *		CloneAddr _FA_((Addr *));
extern char *		ExportType _FA_((int));
extern Addr *		ExtractAddr _FA_((Addr **));
extern char *		ExpTypAddress _FA_((Addr *));
extern char *		ExpTypName _FA_((Addr *));
extern char *		FirstUnTypedName _FA_((Addr *));
extern void		FreeAddr _FA_((Addr **));
extern void		FreeDest _FA_((Dest **));
extern char *		FwdDest _FA_((char *));
extern char *		MapDest _FA_((char *));
extern char *		MapRegion _FA_((char *));
extern char *		MapType _FA_((char *));
extern void		MatchTypes _FA_((Dest *));
extern void		RemoveAddr _FA_((Addr **));
extern char *		SearchNames _FA_((char *));
extern Dest *		SplitDest _FA_((char *));
extern bool		StringAtHome _FA_((char *));
extern Addr *		StringToAddr _FA_((char *, bool));
extern char *		TypedAddress _FA_((Addr *));
extern char *		TypedName _FA_((Addr *));
extern char *		UnTypAddress _FA_((Addr *));
extern char *		UnTypName _FA_((Addr *));

#define	AddressAtHome(A) AddressMatch(A, HomeAddress)
