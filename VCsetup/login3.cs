/*
**	Call script to login at a SUNIII host connected via a direct RS-232 link.
**
**	Imports:
**		[dmnargs]	alternate for localdmnargs
**		linespeed	speed for device
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		passwdstr	password for logina name at remote site
**		ttydevice	device attached to RS-232 line
*/

	set logncount 5;	/* login attempts */
	set logtcount 2;	/* login timeouts */

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;
chkargs:
	match linespeed UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match loginstr UNDEFINED paramerr;
	match ttydevice UNDEFINED paramerr;
	match passwdstr UNDEFINED setpw;
start1:
	timeout 60;
	open "tty" ttydevice "uucplock";
	match RESULT DEVOK openok;
	fail "Could not open " ttydevice ", reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"loginstr=<login name at remote site>\"\n"
		"\t-D \"ttydevice=<device name for connection to remote>\""
		;

setpw:
	set passwdstr "";			/* "passwdstr" defaults to null password */
	next start1;

openok:
	device "speed" linespeed;
	device "flush";

	monitor 2;				/* Turn on I/O tracing */

	sleep 1;				/* Because some terminal i/o is slow! */
	write "\r";
	timeout 15;
loop1:
	expect
		"[Ll]ast login:" loop1,
		"[Ll]ogin:" gotlogin,
		"LOGIN:" gotlogin,
		"incorrect" atlogin,
		"[Pp]assword" atlogin,
		"PASSWORD" atlogin,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT atlogin;

atlogin:
	test logncount lfail;
/*	trace "connected";	*/
retrylogin:
	sleep 1;				/* Because some terminal i/o is slow! */
	write "\r";
	timeout 15;
loop2:
	expect
		"[Ll]ast login:" loop2,
		"[Ll]ogin:" gotlogin,
		"LOGIN:" gotlogin,
		"[Pp]assword" atlogin,
		"PASSWORD" atlogin,
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"PNdaemon starts" startPNdaemon,
		"daemon starts \.\.\. -C" startXNdaemon,
		"daemon starts \.\.\." startNNdaemon,
		"daemon 2 starts \.\.\." startCNdaemon,
		"daemon already active" alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT atlogin;

gotlogin:
/*	trace "login";	*/
	sleep 1;				/* Because some terminal i/o is slow! */
	write loginstr "\r";
	timeout 15;
	expect
		"incorrect" atlogin,
		"[Pp]assword" gotpasswd,
		"PASSWORD" gotpasswd,
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"PNdaemon starts" startPNdaemon,
		"daemon starts \.\.\. -C" startXNdaemon,
		"daemon starts \.\.\." startNNdaemon,
		"daemon 2 starts \.\.\." startCNdaemon,
		"daemon already active" alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT atlogin;

relogin:
/*	trace "login timeout";	*/
	test logtcount rfail;
	next retrylogin;

gotpasswd:
/*	trace "password";	*/
	sleep 1;				/* Because some terminal i/o is slow! */
	write passwdstr "\r";
	set paramcount 4;
	timeout 60;
	expect
		"incorrect" atlogin,
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"PNdaemon starts" startPNdaemon,
		"daemon starts \.\.\. -C" startXNdaemon,
		"daemon starts \.\.\." startNNdaemon,
		"daemon 2 starts \.\.\." startCNdaemon,
		"daemon already active" alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT relogin;

startdaemon:
/*	trace "connection successful";	*/
	forkdaemon localdmnargs;
	return;

alreadyactive:
	fail "remote daemon already active";

eof:
	fail "remote disconnect";

inactive:
	fail "remote network inactive";

lfail:
	fail "too many login attempts";

rfail:
	fail "login timed-out";

shellerr:
	fail "remote " INPUT;

terminate:
	fail "system termination";

startCNdaemon:
	daemon "CNdaemon";
	next startdaemon;

startNNdaemon:
	daemon "NNdaemon";
	next startdaemon;

startPNdaemon:
	daemon "PNdaemon";
	next startdaemon;

startXNdaemon:
	set localdmnargs localdmnargs " -c";
	daemon "NNdaemon";
	next startdaemon;
