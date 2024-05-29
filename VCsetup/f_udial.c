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


#define	STDIO
#define	TIME

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"

#if	UDIAL == 1
#include	"dial.h"
#endif	/* UDIAL == 1 */


static char *	udial_open _FA_((VarArgs *));
static char *	udial_close _FA_((void));



/*
**	Control operations via `udial' uucp dialed circuit.
*/

char *
f_udial(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	Reason = DEVFAIL;

	switch ( dev_op )
	{
	case t_device:
		return tty_device(va);

	case t_open:
		return udial_open(va);

	case t_close:
		return udial_close();
	}

	Fatal2(english("unknown dev_op %d"), dev_op);

	return Reason;
}



/*
**	`close' for `udial' type circuit.
*/

static char *
udial_close()
{
#	if	UDIAL == 1
	undial(Fd);
	Reason = DEVOK;
#	endif	/* UDIAL == 1 */

	Fd = SYSERROR;
	FreeStr(&Fn);

	return Reason;
}


/*
**	`open' for `udial' type circuit.
**
**	Uses the dial(3) C library routine to establish a UUCP
**	style connection. This should be entirely compatible with
**	cu(1), uucp(1) etc. The success of this call will depend on
**	the configuration files for UUCP, located in /usr/lib/uucp.
**	In particular L-devices (Devices and Callers in Honey-Dan-Ber
**	UUCP) need to be set up to describe the autocall units and
**	direct lines to be used.
**
**	Usage:	open "udial" "phonenumber" "speed";
*/

static char *
udial_open(va)
	VarArgs *	va;
{
#	if	UDIAL == 1
	register char *	phone;
	register int	speed;
	char *		err;
	CALL		call;
#	if	SYSVREL < 4
	char		device[DVC_LEN];
#	endif	/* SYSVREL < 4 */

	if ( NARGS(va) == 0 || (phone = ARG(va, 0)) == NULLSTR || phone[0] == '\0' )
	{
		Error(english("phone number?"));
		return DEVFAIL;
	}

	if ( NARGS(va) == 1 || ARG(va, 1) == NULLSTR || (speed = atoi(ARG(va, 1))) == 0 )
	{
		Error(english("line speed?"));
		return DEVFAIL;
	}

	call.telno = phone;
	call.baud = speed;
	call.speed = speed;
	call.modem = TRUE;
	call.attr = (struct termio *) 0;
	call.line = (char *) 0;
#	if	SYSVREL < 4
	call.device = device;
#	else	/* SYSVREL < 4 */
	call.device = NULLSTR;	/* XXX - where is device name returned? */
#	endif	/* SYSVREL < 4 */
	call.dev_len = 0;

	while ( (Fd = dial(call)) < 0 )
	{
		switch ( Fd )
		{
		case INTRPT:
			err = english("interrupt occured");
			break;
		case D_HUNG:
			err = english("dialer hung (no return from write)");
			break;
		case NO_ANS:
			err = english("no answer within 10 seconds");
			break;
		case ILL_BD:
			err = english("illegal baud-rate");
			break;
		case A_PROB:
			err = english("acu problem (open() failure)");
			break;
		case L_PROB:
			err = english("line problem (open() failure)");
			break;
		case NO_Ldv:
			err = english("can't open LDEVS file");
			break;
		case DV_NT_A:
			err = english("requested device not available");
			break;
		case DV_NT_K:
			err = english("requested device not known");
			break;
		case NO_BD_A:
			err = english("no device available at requested baud");
			break;
		case NO_BD_K:
			err = english("no device known at requested baud");
			break;
		case DV_NT_E:
			err = english("requested speed does not match");
			break;
		default:
			err = english("unknown dialler error");
			break;
		}
		Syserror(english("Could not dial %s: %s"), phone, err);
	}

	(void)SetRaw(Fd, 0, 1, 0, false);

	FreeStr(&Fn);
	Fn = newstr(call.device);

	VCtype = BYTESTREAM;

	return DEVOK;

#	else	/* UDIAL == 1 */

	Error(english("dial(3) subroutine unavailable"));
	return DEVFAIL;

#	endif	/* UDIAL == 1 */
}
