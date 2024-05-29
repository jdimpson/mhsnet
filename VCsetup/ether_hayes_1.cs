/*
**	Call script to open an ethernet terminal server modem for dialling
**	(called by "ether_hayes_0.cs").
**
**	This script calls "hayes_2.cs" once for each dialstring specified.
**
**	Imports:
**		[cook]		parameter to turn on circuit cooking ("-c")
**		dialset		string(s) to cause modem to dial remote site (<number>@<speed>[|...])
**		[dmnargs]	extra params for daemon
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		ether_ts	name or IP number for ethernet terminal server
**		ets_username	ethernet terminal server user validation
**		ets_password	ethernet terminal server password validation
**		[fixedspd]	"true" for fixed-speed modem connection
**		[initstr]	initial AT commands for modem
**		[longterm]	seconds for slow terminate
**		loginstr	login name at remote site
**		modem		ether_ts port attached to modem
**		[myscript]	optional script run after modem connects, before unix login
**		[noisy]		"true" for noisy lines: small packets, low throughput
**		opentimeout	TIMEOUT for open
**		openretries	number of open attempts
**		[packettrace]	local packet trace level
**		passwdstr	password for login name at remote site
**		[permanent]	"true" for non-batch mode connections
**		[priority]	max. message priority transmitted
**		[sun3]		expect SUN III daemon
**		type		modem type
*/

	set logncount 3;

	timeout opentimeout;
	retry openretries;
	open "tcp" ether_ts "mhsnet" modem;	/* `modem' == port */
	match RESULT DEVOK	_1_openok;
	set reason "Could not open \"" modem "\" reason: " RESULT;
_1_nextmdm:
	close;					/* Close any open device */
	return;					/* Try next modem */

_1_atlogin:
	test logncount			_1_logfail;
_1_trylogin:
	sleep 1;
	device "flush";
	write "\n";
	timeout 10;
_1_openok:
	match ets_username "^$"		_1_nextdial;
	expect
		"Annex username:"	_1_login,
		"Annex password:"	_1_givepass,
		"WAIT"			_1_allbusy,
		"Login Timed Out"	_1_trylogin,
		"NO CARRIER"		_1_clrmodem,
		"DISCONNECTED"		_1_clrmodem,
		EOF			_1_eof,
		TERMINATE		_1_terminate,
		TIMEOUT			_1_atlogin;

_1_login:
	sleep 1;
	write ets_username "\n";
	expect
		"[Ii]ncorrect"		_1_atlogin,
		"granted"		_1_nextdial,
		"denied"		_1_atlogin,
		"Annex username:"	_1_atlogin,
		"Annex password:"	_1_givepass,
		"Login Timed Out"	_1_atlogin,
		"NO CARRIER"		_1_clrmodem,
		"DISCONNECTED"		_1_clrmodem,
		EOF			_1_eof,
		TERMINATE		_1_terminate,
		TIMEOUT			_1_atlogin;

_1_givepass:
	sleep 1;
	write ets_password "\n";
	expect
		"[Ii]ncorrect"		_1_atlogin,
		"granted"		_1_nextdial,
		"denied"		_1_atlogin,
		"Annex password:"	_1_givepass,
		"Login Timed Out"	_1_atlogin,
		"NO CARRIER"		_1_clrmodem,
		"DISCONNECTED"		_1_clrmodem,
		EOF			_1_eof,
		TERMINATE		_1_terminate,
		TIMEOUT			_1_atlogin;

_1_nextdial:					/* MAIN LOOP STARTS */
	shift dialstr "|" dialset;
	match dialstr UNDEFINED	_1_nextmdm;
	set reason UNDEFINED;
	set savdial dialstr;
	call "_call/hayes_2.cs";		/* Attempt connection */
	match terminate "true"	_1_nextmdm;
	match reason UNDEFINED	_1_nextdial;
	match reason "modem"	_1_nextmdm;	/* Modem error */
	set reason "telno \"" savdial "\" error: " reason;
	trace reason;
	next			_1_nextdial;	/* MAIN LOOP ENDS */

_1_logfail:
	set reason "too many login attempts on Annex";
	return;

_1_clrmodem:
	set reason "NO CARRIER";
	return;

_1_eof:
	set reason "remote disconnect";
	return;

_1_terminate:
	set reason "system termination";
	set terminate "true";
	return;
