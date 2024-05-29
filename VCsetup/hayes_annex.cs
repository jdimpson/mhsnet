/*
**	Call script to login via an `Annex' concentrator (called by "hayes_2.cs").
**	It will return with `reason' UNDEFINED if all ok, and will be succeeded by
**	"hayes_3.cs" to login to the UNIX host.
**
**	Imports:
**		ahost		unix host name on annex
**		[aloginstr]	login name on annex [error if requested and not set]
**		[apasswdstr]	password for login name on annex [default: null]
**		[aport]		MHSnet TCP port on unix host [default: 1989]
*/

	set alogincount	5;			/* Login attempts on Annex */

	/*
	**	Check imported strings.
	*/

	match ahost UNDEFINED		_a_paramerr;
	match aport UNDEFINED		_a_setport;
_a_trylogin0:
	match apasswdstr UNDEFINED	_a_setpw;
_a_trylogin:
	sleep 1;
	device "flush";
	write "\r";
	timeout 10;
_a_loop1:
	expect
		"annex[0-9]*:"		_a_loggedin,
		"Annex username:"	_a_login,
		"Job limit exceeded."	_a_annexerr,
		"WAIT"			_a_allbusy,
		"Login Timed Out"	_a_trylogin,
		"[lL]ogin:"		_a_tryunix,
		"LOGIN:"		_a_tryunix,
		"NO CARRIER"		_a_clrmodem,
		"DISCONNECTED"		_a_clrmodem,
		EOF			_a_eof,
		TERMINATE		_a_terminate,
		TIMEOUT			_a_atlogin;

_a_setpw:
	set apasswdstr "";			/* "apasswdstr" defaults to null password */
	next				_a_trylogin;

_a_setport:
	set aport	1989;			/* `1989' is default MHSnet TCP port */
	next				_a_trylogin0;

_a_paramerr:
	fail "missing one of:\n"
		"\t-D \"aloginstr=<login name on annex>\"\n"
		"\t-D \"ahost=<unix host name on annex>\"\n"
		;

_a_atlogin:
	test alogincount		_a_logfail;
	next				_a_trylogin;

_a_allbusy:
	write "N\r";
	set reason "Annex busy";
	return;

_a_login:
	match aloginstr UNDEFINED	_a_paramerr;
	sleep 1;
	write aloginstr "\r";
	expect
		"Annex username:"	_a_atlogin,
		"Annex password:"	_a_givepass,
		"Login Timed Out"	_a_atlogin,
		"NO CARRIER"		_a_clrmodem,
		"DISCONNECTED"		_a_clrmodem,
		EOF			_a_eof,
		TERMINATE		_a_terminate,
		TIMEOUT			_a_atlogin;

_a_givepass:
	sleep 1;
	write apasswdstr "\r";
	expect
		"[Ii]ncorrect"		_a_atlogin,
		"granted"		_a_loggedin,
		"denied"		_a_atlogin,
		"Annex password:"	_a_givepass,
		"Login Timed Out"	_a_atlogin,
		"NO CARRIER"		_a_clrmodem,
		"DISCONNECTED"		_a_clrmodem,
		EOF			_a_eof,
		TERMINATE		_a_terminate,
		TIMEOUT			_a_atlogin;

_a_loggedin:
/******	For LAT type Annex ports:		*********/
/*	write "stty attn none -imask7 break\r";		*/
/*	sleep 2;					*/
/*	write "connect " ahost "\r";			*/
/******	For non-LAT type Annex ports:		*********/
	write "telnet -t " ahost " " aport "\r";

_a_tryunix:
	set reason UNDEFINED;
	return;					/* Appear to be logged through to unix */

_a_annexerr:
	set reason "Annex error: " INPUT;
	return;

_a_clrmodem:
	set reason "NO CARRIER";
	return;

_a_eof:
	set reason "remote disconnect";
	return;

_a_logfail:
	set reason "too many login attempts on Annex";
	return;

_a_terminate:
	set reason "system termination";
	set terminate "true";
	return;
