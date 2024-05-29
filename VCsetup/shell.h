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


#define	MAX_LINE_LEN	400

/*
**	Structure for VC types by invoked name of shell.
*/

typedef struct VCtype	VCtype;
struct VCtype
{
	char *	name;		/* Program name */
	char *	(*func)_FA_((VCtype *));	/* Function to accept connection */
	char *	args;		/* Name of VC-specific `params' file */
	bool	reply;		/* True if reply expected */
	char *	mesg;		/* Message sent to signal shell startup */
	char *	daemon;		/* Name of daemon */
	char *	params;		/* Params for daemon */
	char *	descrip;	/* Description of circuit type */
	vFuncp	vcp;		/* Optional function to set VC params */
	int	VCrdtype;	/* VC type (BYTESTREAM or DATAGRAM) */
	char *	VCname;		/* Name for circuit, eg `tty` / IP:PORT */
};

/*
**	VC globals.
*/

extern VCtype	Shells[];
