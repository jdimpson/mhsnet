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


#define	FILE_CONTROL
#define	STDIO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"

#	if	XTI == 1
#undef	t_close	
#undef	t_open
#undef	t_device
#include	"xti.h"
#	endif	/* XTI == 1 */

bool	xti_connect = false;	/* set to true if this is an XTI connection */


/*
**	Establish XTI (X/Open Transport Interface - usually X25) connection to remote.
**
**	Must link with: -lxti
*/

char *
f_xti(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
#	if	XTI == 1
	char *		local;
	char *		remote;
	char *		address;
	char *		circuit;
	char *		transport;
	char *		home;
	char		buf[200];
	char		remotebuf[200];
	struct netbuf	t_addr;
	struct netbuf	l_name;
	/* struct t_info	o_info; */
	struct t_bind	r_bind;
	struct t_bind	v_bind;
	struct t_call	s_call;
	struct t_call	r_call;
	int		i;	/* Generic loop variable */
	int		hex;	/* If ==1, circuit type is "hex" and the
				** remote port number is actually a full
				** connection specification.
				*/

	if ( dev_op != vc_open )
	{
		Error(english("only open operation defined for XTI"));
		return DEVFAIL;
	}

	if ( NARGS(va) < 5 || NARGS(va) > 6 )
	{
		Error(english("need XTI local, remote, address, circuit, [service,] home"));
		return DEVFAIL;
	}

	if ( (local = ARG(va, 0)) == NULLSTR )
	{
		Error(english("XTI local?"));
		return DEVFAIL;
	}

	if ( (remote = ARG(va, 1)) == NULLSTR )
	{
		Error(english("XTI remote?"));
		return DEVFAIL;
	}

	if ( (address = ARG(va, 2)) == NULLSTR )
	{
		Error(english("XTI address?"));
		return DEVFAIL;
	}

	if ( (circuit = ARG(va, 3)) == NULLSTR )
	{
		Error(english("XTI circuit designation?"));
		return DEVFAIL;
	}

	if ( NARGS(va) > 5 )
	{
		transport = ARG(va, 4);
	} else {
		transport = "t_osi_cots";
	}

	if ( (home = ARG(va, NARGS(va)-1)) == NULLSTR )
	{
		Error(english("home address?"));
		return DEVFAIL;
	}

	Trace7(1, "f_xti(%s, %s, %s, %s, %s, %s)", local==NULLSTR?NullStr:local, remote==NULLSTR?NullStr:remote, address==NULLSTR?NullStr:address, circuit==NULLSTR?NullStr:circuit, transport==NULLSTR?NullStr:transport, home);

	errno = 0;	/* Clear any pre-existing condition */

	/* Obtain remote TNS information */
	(void)strclr((char *)&t_addr, sizeof(t_addr));
	t_addr.buf = buf;
	t_addr.maxlen = sizeof(buf);

	if ( t_getaddr(remote, &t_addr, (struct netbuf *)0) == SYSERROR )
	{
		address = "t_getaddr";
		goto err;
	}

	t_addr.buf = newnstr(t_addr.buf, t_addr.len);

	Trace3(1, "f_xti() t_getaddr returned: %d=%s", t_addr.len, ExpandString(t_addr.buf, t_addr.len));

	/* Obtain local TNS information */
	(void)strclr((char *)&l_name, sizeof(l_name));
	l_name.buf = buf;
	l_name.maxlen = sizeof(buf);

	if ( t_getloc(local, &l_name, (struct netbuf *)0) == SYSERROR )
	{
		address = "t_getloc";
		goto err;
	}

	l_name.buf = newnstr(l_name.buf, l_name.len);

	Trace3(1, "f_xti() t_getloc returned: %d=%s", l_name.len, ExpandString(l_name.buf, l_name.len));

	/* Obtain file-descriptor for connection */

	Trace2(1, "f_xti() transport is <%s>", transport);
	if ( (Fd = t_open(transport, O_RDWR, NULL)) == SYSERROR )
	{
		address = "t_open";
		goto err;
	}

	/* Trace2(1, "f_xti() t_open returned servtype=%lx", o_info.servtype);*/

	/* Bind `Local Name' to file-descriptor */

	(void)strclr((char *)&r_bind, sizeof(r_bind));
	(void)strclr((char *)&v_bind, sizeof(v_bind));
	r_bind.addr = l_name;

	if ( t_bind(Fd, &r_bind, &v_bind) == SYSERROR )
	{
		address = "t_bind";
		goto err;
	}

	Trace1(1, "f_xti() t_bind returned" );

	/* Connect to `Transport Address' */

	(void)strclr((char *)&s_call, sizeof(s_call));
	(void)strclr((char *)&r_call, sizeof(r_call));
	s_call.addr = t_addr;

	/* Faster than a strcmp... */
	hex = circuit[0]=='h' && circuit[1]=='e' && circuit[2]=='x'
		&& circuit[3]==0;

	/* Ugly but functional.  The 0x42 depends on the device at the far end
	** and is a letter (a or b) identifying the port at the far end.
	** s_call.udata.buf = {0x06,0x02,0x42,0x05,0x00,0x41};
	** Breakdown of the address:
	** 06 - 0 is the length of the calling address, usually 0
	**      6 is the length of the called address in nibbles, usually 6
	** 024205 - the remote address
	** 00 - length of facilities
	** 41 - user data; usually just one character, a lower-case letter.
	**      Fill this in from the first character of "circuit".
	*/
	s_call.udata.buf = remotebuf;
	for (i=0;
	     /* the -5 is -1 (for the leadin byte) - 4 (for 4 bytes padding) */
	     i<sizeof(remotebuf)-5-hex && isxdigit(address[2*i]) &&
				      isxdigit(address[2*i+1]);
	     i++) {
		s_call.udata.buf[i+1-hex] =
			16 * (isdigit(address[2*i]) ? address[2*i] - '0'
						   : (address[2*i] & 0x0f)+9)
			   + (isdigit(address[2*i+1]) ? address[2*i+1] - '0'
						   : (address[2*i+1] & 0x0f)+9);
	}
	if (hex) {
		s_call.udata.len = i;
	}
	else {
		s_call.udata.buf[0] = i*2;
		s_call.udata.len = i+2;
		s_call.udata.buf[s_call.udata.len++] = 0x01;
		s_call.udata.buf[s_call.udata.len++] = 0x00;
		s_call.udata.buf[s_call.udata.len++] = 0x00;
		s_call.udata.buf[s_call.udata.len++] = 0x00;
		if (circuit[0]) {
			s_call.udata.buf[s_call.udata.len++] = circuit[0];
		}
	}
	Trace3(1, "f_xti() t_connect using: %d=%s", s_call.udata.len, ExpandString(s_call.udata.buf, s_call.udata.len));

	if ( t_connect(Fd, &s_call, &r_call) == SYSERROR )
	{
		address = "t_connect";
		goto err;
	}

	Trace1(1, "f_xti() t_connect returned" );

#if 0
	/* We're fairly sure this isn't needed... */
	if ( t_snd(Fd, home, strlen(home)+1) == SYSERROR )
	{
		t_errno = errno;
		address = "write";
		goto err;
	}
#endif

	FreeStr(&Fn);
	Fn = newstr("XTI");

	VCtype = BYTESTREAM;

	xti_connect = true;

	return DEVOK;

err:
	return newprintf("Couldn't %s, t_errno=%d/errno=%d - %s", address, t_errno, errno, t_strerror(t_errno));

#	else	/* XTI == 1 */

	return english("Service not available");

#	endif	/* XTI == 1 */
}
