/*
**	Call script to login and start SUNIII daemons at a remote site.
**
**	Imports:
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		passwdstr	password for login name at remote site
*/

	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite loginstr "\r";
	timeout 60;
_4_loop1:
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"PNdaemon starts"	_4_startPNdaemon,
		"daemon starts ... -C"	_4_startXNdaemon,
		"daemon starts "	_4_startNNdaemon,
		"daemon 2 starts "	_4_startCNdaemon,
		"daemon already active" _4_alreadyactive,
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
	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite passwdstr "\r";
	timeout 60;
_4_loop2:
	expect
		" error -- "		_4_shellerr,
		" ERROR -- "		_4_shellerr,
		"[Nn]o directory"	_4_shellerr,
		"NO DIRECTORY"		_4_shellerr,
		"PNdaemon starts"	_4_startPNdaemon,
		"daemon starts ... -C"	_4_startXNdaemon,
		"daemon starts "	_4_startNNdaemon,
		"daemon 2 starts "	_4_startCNdaemon,
		"daemon already active" _4_alreadyactive,
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

_4_startdaemon:
	device "raw" "10" "1";			/* More efficient reads if SYSV */
	sleep 2;				/* SUN III flushes VC nastily */
	forkdaemon localdmnargs;		/* Transport daemon exits and returns here */
	set reason UNDEFINED;
	set terminate "true";
	return;

_4_terminate:
	set reason "system termination";
_4_exit:
	set terminate "true";
	return;

_4_shellerr:
	set reason "remote: " INPUT;
	next				_4_exit;

_4_eof:
	set reason "remote disconnect";
	return;

_4_alreadyactive:
	set reason "remote daemon already active";
	next				_4_exit;

_4_clrmodem:
	set reason "NO CARRIER";
	return;

_4_startCNdaemon:
	daemon "CNdaemon";
	next _4_startdaemon;

_4_startNNdaemon:
	daemon "NNdaemon";
	next _4_startdaemon;

_4_startPNdaemon:
	daemon "PNdaemon";
	next _4_startdaemon;

_4_startXNdaemon:
	set localdmnargs localdmnargs " -c";
	daemon "NNdaemon";
	next _4_startdaemon;
