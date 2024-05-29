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
**	Array of legal bytes for a region name.
*/

char	Legal_c[] = {

/*	0	1	2	3	4	5	6	7	*/

/*  0*/	0,	0,	0,	0,	0,	0,	0,	0,
/* 10*/	0,	0,	0,	0,	0,	0,	0,	0,
/* 20*/	0,	0,	0,	0,	0,	0,	0,	0,
/* 30*/	0,	0,	0,	0,	0,	0,	0,	0,
/* 40*/	1,	0,	0,	1,	1,	1,	1,	1, /* space # $ % & ' */
/* 50*/	0,	0,	0,	1,	0,	1,	1,	0, /* + - .   */
/* 60*/	1,	1,	1,	1,	1,	1,	1,	1, /* 0-7  */
/* 70*/	1,	1,	0,	0,	0,	1,	0,	1, /* 8 9 = ? */
/*100*/	0,	1,	1,	1,	1,	1,	1,	1, /* A-G  */
/*110*/	1,	1,	1,	1,	1,	1,	1,	1, /* H-O  */
/*120*/	1,	1,	1,	1,	1,	1,	1,	1, /* P-W  */
/*130*/	1,	1,	1,	0,	0,	0,	1,	1, /* X-Z ^ _ */
/*140*/	0,	1,	1,	1,	1,	1,	1,	1, /* a-g  */
/*150*/	1,	1,	1,	1,	1,	1,	1,	1, /* h-o  */
/*160*/	1,	1,	1,	1,	1,	1,	1,	1, /* p-w  */
/*170*/	1,	1,	1,	1,	1,	1,	1,	0, /* x-z { | } ~  */
/*200*/	0,	0,	0,	0,	0,	0,	0,	0,
/*210*/	0,	0,	0,	0,	0,	0,	0,	0,
/*220*/	0,	0,	0,	0,	0,	0,	0,	0,
/*230*/	0,	0,	0,	0,	0,	0,	0,	0,
/*240*/	0,	0,	0,	0,	0,	0,	0,	0,
/*250*/	0,	0,	0,	0,	0,	0,	0,	0,
/*260*/	0,	0,	0,	0,	0,	0,	0,	0,
/*270*/	0,	0,	0,	0,	0,	0,	0,	0,
/*300*/	0,	0,	0,	0,	0,	0,	0,	0,
/*310*/	0,	0,	0,	0,	0,	0,	0,	0,
/*320*/	0,	0,	0,	0,	0,	0,	0,	0,
/*330*/	0,	0,	0,	0,	0,	0,	0,	0,
/*340*/	0,	0,	0,	0,	0,	0,	0,	0,
/*350*/	0,	0,	0,	0,	0,	0,	0,	0,
/*360*/	0,	0,	0,	0,	0,	0,	0,	0,
/*370*/	0,	0,	0,	0,	0,	0,	0,	0
};
