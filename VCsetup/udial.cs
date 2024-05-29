/*
**	Call script to login at a remote host connected via the uucp ``dial'' sub-routine.
**
**	Imports:
**		[dmnargs]	= localdmnargs = remotedmnargs
**		linespeed	speed for device
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		[passwdstr]	password for logina name at remote site
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
**		telno		phone number to call
*/

	monitor 2;		/* Turn on I/O tracing */

	set logncount 5;	/* login attempts */
	set logtcount 2;	/* login timeouts */
	set paramcount 4;	/* params negotiation attempts */

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;
	set remotedmnargs dmnargs;
chkargs:
	match linespeed UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match loginstr UNDEFINED paramerr;
	match telno UNDEFINED paramerr;
	match passwdstr UNDEFINED setpw;
start1:
	match remotedmnargs UNDEFINED setrmda;
start2:
	timeout 60;
	open "udial" telno linespeed;
	match RESULT DEVOK openok;
	close;
	fail "Could not dial " telno ", reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"loginstr=<login name at remote site>\"\n"
		"\t-D \"telno=<phone number to call>\""
		;

setpw:
	set passwdstr "";			/* "passwdstr" defaults to null password */
	next start1;

setrmda:
	set remotedmnargs localdmnargs;		/* "remotedmnargs" defaults to "localdmnargs" */
	next start2;

openok:
	device "speed" linespeed;
	device "flush";

	sleep 1;				/* Because some terminal i/o is slow! */
	timeout 10;
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
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"SHELL STARTS" params,
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
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
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"SHELL STARTS" params,
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
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
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"SHELL STARTS" params,
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT relogin;

sendhome:
	crc val HOMENAME;
	write "HOMENAME " HOMENAME val "\r";
	timeout 6;
	expect
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"SHELL STARTS" params,
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT relogin;

sendpasswd:
	crc val passwdstr;
	write "PASSWORD " passwdstr val "\r";
	timeout 6;
	expect
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"FAILED PASSWORD" failpasswd,
		"QUERY PASSWORD" sendpasswd,
		"SHELL STARTS" params,
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT relogin;

params:
/*	trace "params";	*/
	test paramcount startdefault;
	crc val remotedmnargs;
	write "PARAMS " remotedmnargs val "\r";
	timeout 6;
	expect
		"DEFAULT STARTS" startdefault,
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT params;

startdefault:
	trace "default parameters used";
	set localdmnargs "-c -B -D16 -R300 -S10 -X30";
startdaemon:
/*	trace "connection successful";	*/
	forkdaemon localdmnargs;
	close;
	return;

alreadyactive:
	close;
	fail "remote daemon already active";

eof:
	close;
	fail "remote disconnect";

failpasswd:
	close;
	fail "password incorrect";

failperm:
	close;
	fail "promiscuous connection denied";

inactive:
	close;
	fail "remote network inactive";

lfail:
	close;
	fail "too many login attempts";

rfail:
	close;
	fail "login timed-out";

shellerr:
	close;
	fail "remote " INPUT;

terminate:
	close;
	fail "system termination";
