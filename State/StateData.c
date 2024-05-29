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
#include	"statefile.h"

/*
**	Declare global state processing data.
*/

#undef		Extern
#define		Extern
#define	ExternU
#define		FLAG_DATA

#include	"link.h"

/*
**	Buffer used for commands and read/write statefile.
*/

char	Buf[TOKEN_SIZE+2];
char *	BufEnd = &Buf[TOKEN_SIZE+1];

/*
**	Explanations of name errors.
*/

char	 Ne_floor[]	= english("region below home or link");
char	 Ne_illc[]	= english("illegal char. in address");
char	 Ne_illtype[]	= english("illegal type");
char	 Ne_noname[]	= english("null domain in address");
char	 Ne_notype[]	= english("untyped domain in address");
char	 Ne_null[]	= english("null address");
char	 Ne_toolong[]	= english("line too long");
char	 Ne_unktype[]	= english("unrecognised type");
