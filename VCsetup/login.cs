/*
**	Call script to login at a host connected via a direct RS-232 link.
**
**	Imports:
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		[dmnargs]	= localdmnargs = remotedmnargs
**		linespeed	speed for device
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		[passwdstr]	password for logina name at remote site
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
**		ttydevice	device attached to RS-232 line
*/

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
	match ttydevice UNDEFINED paramerr;
	match passwdstr UNDEFINED setpw;
start1:
	match remotedmnargs UNDEFINED setrmda;
start2:
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

setrmda:
	set remotedmnargs localdmnargs;		/* "remotedmnargs" defaults to "localdmnargs" */
	next start2;

openok:
	device "speed" linespeed;
	device "flush";

	monitor 2;				/* Turn on I/O tracing */

	sleep 1;				/* Because some terminal i/o is slow! */
	write "\r";
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

cookdaemon:
	match localdmnargs "-c "	params_1;
	match localdmnargs "-CX "	params_1;
	set localdmnargs "-CX " localdmnargs;
	set remotedmnargs "-CX " remotedmnargs;
	next				params_1;


useHTdaemon:
	set dmnname "HTdaemon";
namedaemon:
	match dmnname UNDEFINED		useHTdaemon;
	match dmnname "VCdaemon"	sendparams;	/* Don't send the default */
	crc val dmnname;
	write "DAEMON " dmnname val "\r";		/* Expect "QUERY PARAMS" */
	daemon dmnname;
	next				loop3;

vcparams:
	set vcparams "10 1";
	crc val vcparams;
	write "VCCONF " vcparams val "\r";		/* Expect "QUERY DAEMON" */
	next				loop3;

params:
/*	trace "params " INPUT;	*/
	test paramcount startdefault;
	match INPUT "XON_XOFF"		cookdaemon;
params_1:
	match INPUT "STARTS 2V"		vcparams;
	match INPUT "STARTS 2"		namedaemon;
sendparams:
	crc val remotedmnargs;
	write "PARAMS " remotedmnargs val "\r";
loop3:
	timeout 6;
	expect
		"QUERY DAEMON"		namedaemon,
		"QUERY PARAMS"		sendparams,
		"DEFAULT STARTS"	startdefault,
		"DAEMON STARTS"		startdaemon,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT params;

startdefault:
	trace "default parameters used";
	set localdmnargs "-c -B -D16 -R300 -S10 -X30";
startdaemon:
/*	trace "connection successful";	*/
	device "raw" "10" "1";			/* More efficient reads if SYSV */
	forkdaemon localdmnargs;
	return;

alreadyactive:
	fail "remote daemon already active";

eof:
	fail "remote disconnect";

failpasswd:
	fail "password incorrect";

failperm:
	fail "promiscuous connection denied";

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
