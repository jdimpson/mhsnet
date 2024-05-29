/*
**	Call script to call a SUNIII host via a Hayes compatible modem.
**
**	Imports:
**		dialstr		string to cause modem to dial remote site
**		[dialstr2]	optional alternate number to dial remote site
**		[dmnargs]	alternate for localdmnargs
**		[finishstr]	optional initialisation for modem on termination
**		[initstr]	optional initialisation for modem on starting
**		linespeed	speed for device
**		[linespeed2]	speed for optional modemdev2
**		localdmnargs	parameters for daemon at calling site
**		loginstr	login name at remote site
**		modemdev	device attached to modem
**		[modemdev2]	optional alternate device attached to a 2nd modem
**		[passwdstr]	password for login name at remote site [default: null]
**		[xonxoff]	optional boolean that specifies XON/XOFF mode
*/

	set brkcount 3;		/* try auto-ranging login every 3 prompts */
	set dialcount 6;	/* dial attempts */
	set logncount 5;	/* login attempts */
	set logtcount 2;	/* login timeouts */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
chkargs:
	match dialstr UNDEFINED paramerr;
	match linespeed UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match loginstr UNDEFINED paramerr;
	match modemdev UNDEFINED paramerr;
	match passwdstr UNDEFINED setpw;
start1:
	monitor 2;				/* Turn on I/O tracing */

	retry 60;				/* Allow 60 attempts at open */
	timeout 60;				/* Wait 1 minute between/for open attempts */
	open "dial" modemdev "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK openok;
	set reason "Could not open \"" modemdev "\" reason: " RESULT;
nextmodem:
	match modemdev2 UNDEFINED openfail;
	trace reason;
	set reason UNDEFINED;
	set modemdev modemdev2;
	set modemdev2 UNDEFINED;
	match linespeed2 UNDEFINED start1;
	set linespeed modemdev2;
	set linespeed2 UNDEFINED;
	next start1;

setpw:
	set passwdstr "";			/* "passwdstr" defaults to null password */
	next start1;

openfail:
	fail reason;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dialstr=<ATD command for modem>\"\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"loginstr=<login name at remote site>\"\n"
		"\t-D \"modemdev=<device pathname for modem>\""
		;

openok:
	device "offdelay";			/* Otherwise reads will return 0 */
	device "speed" linespeed;
	match xonxoff UNDEFINED no_xonxoff;
	device "xonxoff";
no_xonxoff:

	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite "ATE0Q0V1S0=0S2=43\r";
	timeout 10;
	expect
		"OK" try_initstr,
		"ERROR" modem_err,
		"[Ll]ogin:" gotlogin,
		"LOGIN:" gotlogin,
		"[Pp]assword" atlogin,
		"PASSWORD" atlogin,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT rstmodem;

rstmodem:
	sleep 2;
	slowwrite "+++";
clrmodem:
	sleep 2;
	slowwrite "ATE0Q0V1S0=0S2=43\r";
	timeout 10;
	expect
		"OK" try_initstr,
		"ERROR" modem_err,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT nomodem;

try_initstr:
	match initstr UNDEFINED dialing;
	timeout 0;
	sleep 1;
	slowwrite initstr "\r";
	timeout 10;
	expect
		"OK" dialing,
		"ERROR" modem_err,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT dialing;

nomodem:
	close;
	set reason "modem \"" modemdev "\" not responding";
	next nextmodem;

dialing:
	test dialcount dfail;
	timeout 0;
	sleep 1;
	slowwrite dialstr "\r";
	timeout 60;
	expect
		"BUSY" busy,
		"NO CARRIER" clrmodem,
		"NO DIALTONE" nodial,
		"CONNECT 9600" got9600,
		"CONNECT 4800" got4800,
		"CONNECT 2400" got2400,
		"CONNECT 1200" got1200,
		"CONNECT 300" got300,
		"CONNECT" trylogin,
		"ERROR" modem_err,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT rstmodem;

busy:
	trace "number \"" dialstr "\" busy";
	match dialstr2 UNDEFINED busy2;
nextnumber:
	set dialstr dialstr2;
	set dialstr2 UNDEFINED;
	set dialcount 6;
	next dialing;
busy2:
	sleep 30;				/* Give them time to get off the phone */
	next clrmodem;

sendbreak:
	device "break";
	set brkcount 3;
	next trylogin;

trybreak:
	test brkcount sendbreak;
atlogin:
	test logncount lfail;
trylogin:
	device "remote";			/* Attend to modem signals */
	sleep 1;
	slowwrite "\r";
	timeout 15;
loop1:
	expect
		"[Ll]ast login:" loop1,
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
		"NO CARRIER" clrmodem,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT trybreak;

got9600:
	set localdmnargs localdmnargs " -nD256 -S400 -X960";
	device "speed" "9600";
	next trylogin;

got4800:
	set localdmnargs localdmnargs " -D256 -S200 -X480";
	device "speed" "4800";
	next trylogin;

got2400:
	set localdmnargs localdmnargs " -D128 -S100 -X240";
	device "speed" "2400";
	next trylogin;

got1200:
	set localdmnargs localdmnargs " -D64 -S50 -X120";
	device "speed" "1200";
	next trylogin;

got300:
	set localdmnargs localdmnargs " -D16 -S10 -X30";
	device "speed" "300";
	next trylogin;

gotlogin:
	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite loginstr "\r";
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
		"NO CARRIER" clrmodem,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT atlogin;

relogin:
	test logtcount rfail;
	next trylogin;

gotpasswd:
	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite passwdstr "\r";
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
		"NO CARRIER" clrmodem,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT relogin;

startdaemon:
	sleep 2;				/* SUN III flushes VC nastily */
	forkdaemon localdmnargs;		/* Transport daemon exits and returns here */
endmodem:
	device "local";				/* Ignore modem signals */
	sleep 2;
	device "flush";
	slowwrite "+++";
	timeout 3;
	expect
		"NO CARRIER" endmodem1,
		"OK" endmodem1,
		"ERROR" modem_err,
		EOF endmodem2,
		TERMINATE endmodem0,
		TIMEOUT endmodem1;

endmodem0:
	sleep 2;
	device "flush";
endmodem1:
	slowwrite "ATH0\r";
	timeout 2;
	expect
		"OK" endmodem2,
		"ERROR" modem_err,
		EOF endmodem2,
		TERMINATE endmodem2,
		TIMEOUT endmodem2;

endmodem2:
	timeout 0;
	match finishstr ".+" endmodem3;
	set finishstr "ATE0Q1S0=1";
endmodem3:
	slowwrite finishstr "\r";
	sleep 1;
	match reason UNDEFINED out;
	fail reason;
out:
	return;

alreadyactive:
	set reason "remote daemon already active";
	next endmodem;

dfail:
	match dialstr2 UNDEFINED dfail2;
	trace "number \"" dialstr "\" busy, trying \"" dialstr2 "\"";
	next nextnumber;
dfail2:	
	set reason "too many dial attempts";
	next endmodem;

eof:
	set reason "remote disconnect";
	next endmodem;

inactive:
	set reason "remote network inactive";
	next endmodem;

lfail:
	set reason "too many login attempts";
	next endmodem;

modem_err:
	set reason "modem \"" modemdev "\" command error";
	next nextmodem;

nodial:
	close;
	set reason "modem \"" modemdev "\" has no dialtone";
	match modemdev2 UNDEFINED endmodem;
	next nextmodem;

rfail:
	set reason "login timed-out";
	next endmodem;

shellerr:
	set reason "remote: " INPUT;
	next endmodem;

terminate:
	set reason "system termination";
	next endmodem;

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
