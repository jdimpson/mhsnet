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


#include	"global.h"
#include	"debug.h"
#include	"licence.h"

#include	<ctype.h>


static Time_t	decode _FA_((char *));



/*
**	Check licence number.
*/

int
CheckLicence(site, licence, links, linkname)
	char *		site;		/* Internal form site address */
	char *		licence;	/* Activation key / serial number */
	int		links;		/* Number of links */
	char *		linkname;	/* Name of first link */
{
	register int	keylen;
	register char *	cp;
	register int	c;

	if ( (cp = licence) == NULLSTR )
		return 1;

	for ( keylen = 0 ; (c = *cp++) != '\0' && isalpha(c) ; keylen++ );

	if ( keylen < LICENCE_CHECK_LENGTH )
	{
		if ( keylen == 0 )
			return LICENCE_BAD;	/* No licence */

		Error(english("unrecognised licence number \"%s\""), licence);
	}

	cp = &licence[LICENCE_CHECK_LENGTH];

	if
	(
		*cp == LICENCE_CHAR_DATE
		&&
		decode(cp+1) < Time
	)
		return LICENCE_EXPIRED;		/* Expired */

	if ( strchr(cp, LICENCE_CHAR_LINK) == NULLSTR )
		linkname = EmptyString;
	else
	if ( links != 1 )
		return LICENCE_LINK;		/* Wrong number of links */

	cp = MakeLicence(site, linkname, cp, LICENCE_CHECK_LENGTH);

	return strnccmp(licence, cp, LICENCE_CHECK_LENGTH);	/* Largest +ve result is 0377 */
}



/*
**	Make activation key from site/serial number.
*/

char *
MakeLicence(site, link, serial, length)
	char *		site;		/* Internal form site address */
	char *		link;		/* Internal form link address (if link restricted) */
	char *		serial;		/* Serial number of licencee */
	int		length;		/* Number of bytes in activation key */
{
	register int	i;
	register Ulong	x;
	register int	sitelen;
	register int	linklen;
	register int	serilen;
	static char	key[LICENCE_CHECK_LENGTH+1];

	sitelen = strlen(site);

	site = ToLower(site, sitelen);

	if ( (linklen = strlen(link)) > 0 )
		link = ToLower(link, linklen);

	serilen = strlen(serial);
	serial = ToUpper(serial, serilen);

	if ( length == 4 )	/* OLD */
	{
		x = (Ulong)acrc((Crc_t)0, site, sitelen);
		x = (Ulong)acrc((Crc_t)x, serial, serilen);
	}
	else
	if ( length == LICENCE_CHECK_LENGTH )
	{
		x = (Ulong)acrc32((Crc32_t)0, site, sitelen);
		if ( linklen > 0 )
			x = (Ulong)acrc32((Crc32_t)x, link, linklen);
		x = (Ulong)acrc32((Crc32_t)x, serial, serilen);
	}
	else
	{
		x = 0;	/* Quieten -Wall */
		Fatal2(english("bad licence length: %d"), length);
	}

	key[length] = '\0';
	for ( i = length ; --i >= 0 ; )
	{
		key[i] = (x&0xf)+'A';
		x >>= 4;
	}

	free(serial);
	if ( linklen > 0 )
		free(link);
	free(site);

	return key;
}



/*
**	Convert A-P format string to unsigned long integer.
*/

static Time_t
decode(s)
	register char *	s;
{
	register Ulong	n;
	register int	c;
	register int	yet;

	for ( n = 0, yet = 0 ; (c = *s++) != 0 ; )
		switch ( c )
		{
		case ' ': case '\t':	/* Ignore leading white space */
			if ( !yet )
				continue;
			return n;
		default:
			switch ( c |= 040 )
			{
			case 'a': case 'b': case 'c': case 'd':
			case 'e': case 'f': case 'g': case 'h':
			case 'i': case 'j': case 'k': case 'l':
			case 'm': case 'n': case 'o': case 'p':
				n <<= 4;
				n += c - 'a';
				yet = 1;
				continue;
			default:
				return n;
			}
		}

	return n;
}
