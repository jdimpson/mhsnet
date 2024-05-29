/*
**	Call script to login and start daemons at a remote site (called by "hayes_3.cs").
**
**	Imports:
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		passwdstr	password for login name or netpassword at remote site
**		remotedmnargs	parameters for daemon at remote site
**		speed		bps to modem
*/

	set paramcount	11;			/* Parameters negotiation attempts */
	set maxloop	10;			/* QUERY loop at _4_loop */

	set vmin "10";				/* More efficient reads if SYSV */
	match speed
		"38400"		_4_vmin384,
		"19200"		_4_vmin192,
		"9600"		_4_vmin96,
		"2400"		_4_vmin24,
		".*"		_4_start;

_4_vmin384:
	set vmin "400";
	next			_4_start;

_4_vmin192:
	set vmin "200";
	next			_4_start;

_4_vmin96:
	set vmin "100";
	next			_4_start;

_4_vmin24:
	set vmin "30";

_4_start:
	match INPUT				/* In case already logged in */
		"QUERY HOMENAME"	_4_sendhome,
		"SHELL STARTS"		_4_params;

	write loginstr "\r";
	timeout 20;
_4_loop1:
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"Could not exec"	_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"QUERY HOMENAME"	_4_sendhome,
		"CONNECTION DISALLOWED"	_4_failperm,
		"QUERY PASSWORD"	_4_sendpasswd,
		"SHELL STARTS"		_4_params,
		"DEFAULT STARTS"	_4_startdefault,
		"DAEMON ALREADY ACTIVE"	_4_alreadyactive,
		"NETWORK INACTIVE"	_4_inactive,
		"NO CARRIER"		_4_clrmodem,
		"[Ll]ast login:"	_4_loop1,
		"LAST LOGIN:"		_4_loop1,
		"[Ll]ogin:"		_4_atlogin,
		"LOGIN:"		_4_atlogin,
		"incorrect"		_4_atlogin,
		"INCORRECT"		_4_atlogin,
		"[Pp]assword"		_4_gotpasswd,
		"PASSWORD"		_4_gotpasswd,
		EOF			_4_eof,
		TERMINATE		_4_terminate,
		TIMEOUT			_4_atlogin;

_4_atlogin:
	return;

_4_gotpasswd:
	write passwdstr "\r";
	timeout 60;
_4_loop2:
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"Could not exec"	_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"QUERY HOMENAME"	_4_sendhome,
		"CONNECTION DISALLOWED"	_4_failperm,
		"QUERY PASSWORD"	_4_sendpasswd,
		"SHELL STARTS"		_4_params,
		"DEFAULT STARTS"	_4_startdefault,
		"DAEMON ALREADY ACTIVE"	_4_alreadyactive,
		"NETWORK INACTIVE"	_4_inactive,
		"NO CARRIER"		_4_clrmodem,
		"[Ll]ast login:"	_4_loop2,
		"LAST LOGIN:"		_4_loop2,
		"[Ll]ogin:"		_4_atlogin,
		"LOGIN:"		_4_atlogin,
		"incorrect"		_4_atlogin,
		"INCORRECT"		_4_atlogin,
		"[Pp]assword"		_4_atlogin,
		"PASSWORD"		_4_atlogin,
		EOF			_4_eof,
		TERMINATE		_4_terminate,
		TIMEOUT			_4_atlogin;

_4_sendhome:
	crc val HOMENAME;
	write "HOMENAME " HOMENAME val "\r";
	timeout 10;
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"Could not exec"	_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"QUERY HOMENAME"	_4_sendhome,
		"CONNECTION DISALLOWED"	_4_failperm,
		"QUERY PASSWORD"	_4_sendpasswd,
		"SHELL STARTS"		_4_params,
		"DEFAULT STARTS"	_4_startdefault,
		"DAEMON STARTS"		_4_startdaemon,
		"DAEMON ALREADY ACTIVE"	_4_alreadyactive,
		"NETWORK INACTIVE"	_4_inactive,
		"NO CARRIER"		_4_clrmodem,
		"[Ll]ogin:"		_4_atlogin,
		"LOGIN:"		_4_atlogin,
		EOF			_4_eof,
		TERMINATE		_4_terminate,
		TIMEOUT			_4_sendhome;

_4_sendpasswd:
	set loopcounter maxloop;
	set xmitstr1 "PASSWORD ";
	set xmitstr2 passwdstr;
_4_loop:
	crc val xmitstr2;
	write xmitstr1 xmitstr2 val "\r";
	timeout 10;
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"Could not exec"	_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"FAILED PASSWORD"	_4_failpasswd,
		"QUERY PASSWORD"	_4_sendpasswd,
		"SHELL STARTS"		_4_params,
		"QUERY DAEMON"		_4_namedaemon,
		"QUERY PARAMS"		_4_sendparams,
		"DEFAULT STARTS"	_4_startdefault,
		"DAEMON STARTS"		_4_startdaemon,
		"DAEMON ALREADY ACTIVE"	_4_alreadyactive,
		"NETWORK INACTIVE"	_4_inactive,
		"NO CARRIER"		_4_clrmodem,
		"[Ll]ogin:"		_4_atlogin,
		"LOGIN:"		_4_atlogin,
		EOF			_4_eof,
		TERMINATE		_4_terminate,
		TIMEOUT			_4_cntloop;

_4_cntloop:
	test loopcounter		_4_looperr;
	next				_4_loop;

_4_cookdaemon:
	match localdmnargs "-c "	_4_params_1;
	match localdmnargs "-CX "	_4_params_1;
	set localdmnargs "-CX " localdmnargs;
	set remotedmnargs "-CX " remotedmnargs;
	next				_4_params_1;

_4_useHTdaemon:
	set dmnname "HTdaemon";
_4_namedaemon:
	match dmnname UNDEFINED		_4_useHTdaemon;
	match dmnname "VCdaemon"	_4_sendparams;	/* Don't send the default */
	daemon dmnname;
	set loopcounter maxloop;
	set xmitstr1 "DAEMON ";
	set xmitstr2 dmnname;
	next				_4_loop;

_4_vcparams:
	set loopcounter maxloop;
	set xmitstr1 "VCCONF ";
	set xmitstr2 vmin;
	next				_4_loop;

_4_params:
	test paramcount			_4_startdefault;
	match INPUT "XON_XOFF"		_4_cookdaemon;
_4_params_1:
	match INPUT "STARTS 2V"		_4_vcparams;
	match INPUT "STARTS 2"		_4_namedaemon;
_4_sendparams:
	crc val remotedmnargs;
	write "PARAMS " remotedmnargs val "\r";
	timeout 10;
	expect
		"SHELL STARTS"		_4_params,
		"QUERY DAEMON"		_4_namedaemon,
		"QUERY PARAMS"		_4_sendparams,
		"DEFAULT STARTS"	_4_startdefault,
		"DAEMON STARTS"		_4_startdaemon,
		"DAEMON ALREADY ACTIVE"	_4_alreadyactive,
		"NO CARRIER"		_4_clrmodem,
		"[Ll]ogin:"		_4_atlogin,
		"LOGIN:"		_4_atlogin,
		EOF			_4_eof,
		TERMINATE		_4_terminate,
		TIMEOUT			_4_cntparams;

_4_cntparams:
	test loopcounter		_4_looperr;
	next				_4_sendparams;

_4_startdefault:
	trace "WARNING: default parameters used";
	set localdmnargs "-cB5 -D16 -R300 -S10 -X30";
_4_startdaemon:
	device "raw" vmin;
	forkdaemon localdmnargs;		/* Transport daemon exits and returns here */
	set reason UNDEFINED;
_4_exit:
	set terminate "true";
	return;

_4_terminate:
	set reason "system termination";
	next				_4_exit;

_4_alreadyactive:
	set reason "remote daemon already active";
	next				_4_exit;

_4_clrmodem:
	set reason "NO CARRIER";
	return;

_4_eof:
	set reason "remote disconnect";
	return;

_4_failpasswd:
	set reason "password incorrect";
	next				_4_exit;

_4_failperm:
	set reason "promiscuous connection denied";
	next				_4_exit;

_4_inactive:
	set reason "remote network inactive";
	next				_4_exit;

_4_looperr:
	set reason "too many parameter attempts";
	return;

_4_shellerr:
	set reason "remote: " INPUT;
	next				_4_exit;
