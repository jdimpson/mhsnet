/*
**	Call script to get a login prompt at a remote site (called by "hayes_2.cs").
**
**	This script calls "hayes_4.cs" or "hayes_4_3.cs" to perform login
**	(depending on the setting of "sun3").
**
**	Imports:
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		passwdstr	password for login name at remote site
**		remotedmnargs	parameters for daemon at remote site
**		speed		bps to modem
**		[sun3]		use "hayes_4_3.cs" next
*/

	set brkcount	 4;			/* auto-ranging getty - 4 speeds? */
	set logncount	 2;			/* login attempts @ each speed */

	match ether_ts "."		_3_trylogin1;	/* Avoid device control */
	match VERSION "BSDI"		_3_trylogin1;
	device "remote";			/* Attend to modem signals ** NOT BSDI! */
	next				_3_trylogin1;

_3_trylogin:
	sleep 1;
	device "flush";
	write "\r";
_3_trylogin1:
	timeout 10;
_3_loop1:
	expect
		"[Ll]ast login:"	_3_loop1,
		"LAST LOGIN:"		_3_loop1,
		"[Ll]ogin:"		_3_gotlogin,
		"LOGIN:"		_3_gotlogin,
		"[Pp]assword"		_3_trylogin,
		"PASSWORD"		_3_trylogin,
		"SHELL STARTS"		_3_gotlogin,
		"QUERY HOMENAME"	_3_gotlogin,
		"NO CARRIER"		_3_clrmodem,
		EOF			_3_eof,
		TERMINATE		_3_terminate,
		TIMEOUT			_3_atlogin;

_3_gotlogin:
	match sun3 UNDEFINED		_3_gotlogin4;
	match sun3 "false"		_3_gotlogin4;
	call "_call/hayes_4_3.cs";		/* SUN III login */
	next				_3_endlogin;
_3_gotlogin4:
	call "_call/hayes_4.cs";		/* MHSnet login */
_3_endlogin:
	match terminate "true"		_3_exit;
	match reason UNDEFINED		_3_atlogin;
_3_exit:
	return;

_3_atlogin:
	test logncount			_3_sendbreak;
	next				_3_trylogin;

_3_sendbreak:
	match ether_ts "."		_3_lfail;
	test brkcount			_3_lfail;
	device "break";
	set logncount 2;
	next				_3_trylogin;

_3_lfail:
	set reason "too many login attempts";
	return;

_3_clrmodem:
	set reason "NO CARRIER";
	return;

_3_eof:
	set reason "remote disconnect";
	return;

_3_terminate:
	set reason "system termination";
	set terminate "true";
	return;
