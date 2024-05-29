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
**	SUN3 defines for use by gateway code.
*/

#define	SUN4_3SPOOLER	"Sun4_3"
#define	SUN3PARAMS	"sun3"
#define	NOTSUN3		"NOTSUN3"
#define	NOTSUN4		"NOTSUN4"

#define	GLOBALDEST	"*.oz.au"

#define	Sun3ERs		'\036'
#define	Sun3EUs		'='
#define	Sun3RS		','
#define	Sun3RT		'='

#define	ENV3_ACK	"ACK"
#define	ENV3_DESTINATION "DEST"
#define	ENV3_DOWN	"DOWN"
#define	ENV3_DUP	"DUP"
#define	ENV3_ENQ	"ENQ"
#define	ENV3_ENQALL	"ENQALL"
#define	ENV3_ERR1	"ERR1"
#define	ENV3_ERR2	"ERR2"
#define	ENV3_HANDLER_FLAGS "FLAGS"
#define	ENV3_ID		"ID"
#define	ENV3_NO_AUTOCALL "NOCALL"
#define	ENV3_NOOPT	"NOOPT"
#define	ENV3_NORET	"NORET"
#define	ENV3_NOTNODE	"NOTNODE"
#define	ENV3_RETURNED	"RET"
#define	ENV3_REV_CHARGE	"R_C"
#define	ENV3_ROUTE	"ROUTE"
#define	ENV3_STATE	"STATE"
#define	ENV3_TRUNC	"TRUNC"
#define	ENV3_UP		"UP"

/*
**	Externals.
*/

extern char		Au[];
extern char		OzAu[];
