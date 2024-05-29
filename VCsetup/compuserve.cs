/*
**	Call script to call a Compuserve host
**	via a Hayes compatible modem.
**
**	Imports:
**		clogin		login name at Compuserve
**		[cpasswd]	Compuserve password [default: null]
**		dialstr		string to cause modem to dial remote site
**		[dialstr2]	optional alternate number to dial remote site
**		[dmnargs]	sets localdmnargs & remotedmnargs
**		[finishstr]	optional initialisation for modem on termination
**		[initstr]	optional initialisation for modem on starting
**		linespeed	speed for modemdev
**		[linespeed2]	speed for optional modemdev2
**		localdmnargs	parameters for daemon at calling site
**		modemdev	device attached to modem
**		[modemdev2]	optional alternate device attached to a 2nd modem
**		[npasswd]	network password at remote site [default: upasswd]
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
**		ulogin		login name at remote site
**		[upasswd]	password for login name at remote site [default: null]
**		[xonxoff]	optional boolean that specifies XON/XOFF mode
*/

	monitor 2;		/* Turn on I/O tracing */

	set brkcount 2;		/* try auto-ranging login every 3 prompts */
	set dialcount 3;	/* dial attempts */
	set logncount 5;	/* login attempts */
	set logtcount 2;	/* login timeouts */
	set paramcount 11;	/* params negotiation attempts */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
	set remotedmnargs dmnargs;
chkargs:
	match clogin UNDEFINED		paramerr;
	match dialstr UNDEFINED		paramerr;
	match linespeed UNDEFINED	paramerr;
	match localdmnargs UNDEFINED	paramerr;
	match modemdev UNDEFINED	paramerr;
	match ulogin UNDEFINED		paramerr;
	match cpasswd UNDEFINED 	setcpw;
start0:
	match upasswd UNDEFINED		setupw;
start01:
	match npasswd UNDEFINED		setnpw;
start1:
	match remotedmnargs UNDEFINED	setrmda;
start2:
	set savlcldmnargs localdmnargs;
	set savrmtdmnargs remotedmnargs;
	timeout 60;				/* Wait 1 minute between/for open attempts */
	retry 60;				/* Allow 60 attempts at open => 1 hour max */
	open "dial" modemdev "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK		openok;
	set reason "Could not open \"" modemdev "\" reason: " RESULT;
nextmodem:
	match modemdev2 UNDEFINED	openfail;
	close;					/* Close first device */
	trace reason;
	set reason UNDEFINED;
	set modemdev modemdev2;
	set modemdev2 UNDEFINED;
	match linespeed2 UNDEFINED	start2;
	set linespeed modemdev2;
	set linespeed2 UNDEFINED;
	next				start2;

setcpw:
	set cpasswd "";				/* "cpasswd" defaults to null password */
	next				start0;

setnpw:
	set npasswd upasswd;			/* "npasswd" defaults to "upasswd" */
	next				start1;

setupw:
	set upasswd "";				/* "upasswd" defaults to null password */
	next				start01;

setrmda:
	set remotedmnargs localdmnargs;		/* "remotedmnargs" defaults to "localdmnargs" */
	next				start2;

openfail:
	fail reason;

paramerr:
	fail "missing some of:\n"
		"\t-D \"clogin=<login name at Compuserve>\"\n"
		"\t-D \"dialstr=<ATD command for modem>\"\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"ulogin=<login name at remote site>\"\n"
		"\t-D \"modemdev=<device pathname for modem>\""
		;

openok:
	device "offdelay";			/* Otherwise reads will return 0 */
	device "speed" linespeed;
	match xonxoff UNDEFINED		no_xonxoff;
	device "xonxoff";
no_xonxoff:
	match linespeed "19200" rtscts;
	next				skip_rtscts;
rtscts:
	match VERSION "XENIX"		setrts_xenix;
	match VERSION "BSDI"		setrts_bsdi;
	trace "delete one of the following, or add your own RTS/CTS setup here ...";
	device "stty" "crtscts";
	match RESULT DEVOK		skip_rtscts;
	device "stty" "rtscts";
	match RESULT DEVOK		skip_rtscts;
setrts_error:
	trace "stty to set RTS/CTS flow-control failed";
	next				skip_rtscts;

setrts_bsdi:
	device "stty" "rts_iflow cts_oflow";
	match RESULT DEVOK		skip_rtscts;
	next				setrts_error;

setrts_xenix:
	device "stty" "rtsflow ctsflow";
	match RESULT DEVOK		skip_rtscts;
	next				setrts_error;

skip_rtscts:
	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite "ATE0Q0V1S0=0S2=43\r";
	timeout 10;
	expect
		"OK"			try_initstr,
		"ERROR"			modem_err,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			rstmodem;

rstmodem:
	device "local";
	sleep 2;
	device "flush";
	slowwrite "+++";
	timeout 10;
	expect
		"OK"			clrmodem,
		"ERROR"			modem_err,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			clrmodem;
clrmodem:
	sleep 2;
	slowwrite "ATE0Q0V1S0=0S2=43H0\r";
	timeout 10;
	expect
		"OK"			try_initstr,
		"ERROR"			modem_err,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			nomodem;

try_initstr:
	match initstr UNDEFINED dialing;
	timeout 0;
	sleep 1;
	slowwrite initstr "\r";
	timeout 10;
	expect
		"OK"			dialing,
		"ERROR"			modem_err,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			dialing;

modem_err:
	set reason "modem \"" modemdev "\" command error";
	next				nextmodem;

nomodem:
	set reason "modem \"" modemdev "\" not responding";
	next				nextmodem;

dialing:
	test dialcount dfail;
	timeout 0;
	sleep 1;
	slowwrite dialstr "\r";
	timeout 60;
	expect
		"BUSY"			busy,
		"NO ANSWER"		busy,
		"NO CARRIER"		clrmodem,
		"NO DIALTONE"		nodial,
		"CONNECT 9600"		got9600,
		"CONNECT 4800"		got4800,
		"CONNECT 2400"		got2400,
		"CONNECT 1200"		got1200,
		"CONNECT 300"		got300,
		"CONNECT"		trylogin,
		"ERROR"			modem_err,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			rstmodem;

busy:
	trace "number \"" dialstr "\" busy";
	match dialstr2 UNDEFINED busy2;
nextnumber:
	set dialstr dialstr2;
	set dialstr2 UNDEFINED;
	set dialcount 3;
	next				dialing;
busy2:
	sleep 30;				/* Give them time to get off the phone */
	next				clrmodem;

sendbreak:
	device "break";
	set brkcount 3;
	next				trylogin;

trybreak:
	test brkcount sendbreak;
	test logncount lfail;
trylogin:
	device "remote";			/* Attend to modem signals */
	sleep 1;
	slowwrite "\r";				/* Wakeup Compuserve */
	timeout 15;
	expect
		"Host"			tryclogin,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			trybreak;

tryclogin:
	sleep 1;
	slowwrite clogin "\r";
	timeout 45;
loop0:
	expect
		"word:"			trycpass,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			trybreak;

trycpass:
	sleep 1;
	slowwrite cpasswd "\r";
	set logncount 5;
	timeout 45;
loop1:
	expect
		"[Ll]ast login:"	loop1,
		"LAST LOGIN:"		loop1,
		"[Ll]ogin:"		gotlogin,
		"LOGIN:"		gotlogin,
		"[Pp]assword"		atlogin,
		"PASSWORD"		atlogin,
		" error -- "		shellerr,
		" ERROR -- "		shellerr,
		"[Nn]o directory"	shellerr,
		"NO DIRECTORY"		shellerr,
		"QUERY HOMENAME"	sendhome,
		"CONNECTION DISALLOWED"	failperm,
		"QUERY PASSWORD"	sendpasswd,
		"SHELL STARTS"		params,
		"DAEMON STARTS"		startdaemon,
		"DEFAULT STARTS"	startdefault,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NETWORK INACTIVE"	inactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			trybreak;

atlogin:
	test logncount lfail;
	slowwrite "\r";
	next				loop1;

got9600:
	set localdmnargs savlcldmnargs " -D256 -S100 -X960";
	set remotedmnargs savrmtdmnargs " -D256 -S100 -X960";
	device "speed" "9600";
	next				trylogin;

got4800:
	set localdmnargs savlcldmnargs " -D128 -S100 -X480";
	set remotedmnargs savrmtdmnargs " -D128 -S100 -X480";
	device "speed" "4800";
	next				trylogin;

got2400:
	set localdmnargs savlcldmnargs " -D64 -S50 -X240";
	set remotedmnargs savrmtdmnargs " -D64 -S50 -X240";
	device "speed" "2400";
	next				trylogin;

got1200:
	set localdmnargs savlcldmnargs " -D32 -S25 -X120";
	set remotedmnargs savrmtdmnargs " -D32 -S25 -X120";
	device "speed" "1200";
	next				trylogin;

got300:
	set localdmnargs savlcldmnargs " -D16 -S10 -X30";
	set remotedmnargs savrmtdmnargs " -D16 -S10 -X30";
	device "speed" "300";
	next				trylogin;

gotlogin:
	sleep 1;
	slowwrite ulogin "\r";
	timeout 60;
	expect
		"incorrect"		atlogin,
		"INCORRECT"		atlogin,
		"[Pp]assword"		gotpasswd,
		"PASSWORD"		gotpasswd,
		" error -- "		shellerr,
		" ERROR -- "		shellerr,
		"[Nn]o directory"	shellerr,
		"NO DIRECTORY"		shellerr,
		"QUERY HOMENAME"	sendhome,
		"CONNECTION DISALLOWED"	failperm,
		"QUERY PASSWORD"	sendpasswd,
		"SHELL STARTS"		params,
		"DAEMON STARTS"		startdaemon,
		"DEFAULT STARTS"	startdefault,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NETWORK INACTIVE"	inactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			atlogin;

relogin:
	test logtcount rfail;
	next				trylogin;

gotpasswd:
	sleep 1;
	slowwrite upasswd "\r";
	set paramcount 11;
	timeout 60;
	expect
		"incorrect"		atlogin,
		"INCORRECT"		atlogin,
		" error -- "		shellerr,
		" ERROR -- "		shellerr,
		"[Nn]o directory"	shellerr,
		"NO DIRECTORY"		shellerr,
		"QUERY HOMENAME"	sendhome,
		"CONNECTION DISALLOWED"	failperm,
		"QUERY PASSWORD"	sendpasswd,
		"SHELL STARTS"		params,
		"DAEMON STARTS"		startdaemon,
		"DEFAULT STARTS"	startdefault,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NETWORK INACTIVE"	inactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			relogin;

sendhome:
	crc val HOMENAME;
	write "HOMENAME " HOMENAME val "\r";
	timeout 10;
	expect
		" error -- "		shellerr,
		" ERROR -- "		shellerr,
		"[Nn]o directory"	shellerr,
		"NO DIRECTORY"		shellerr,
		"QUERY HOMENAME"	sendhome,
		"CONNECTION DISALLOWED"	failperm,
		"QUERY PASSWORD"	sendpasswd,
		"SHELL STARTS"		params,
		"DAEMON STARTS"		startdaemon,
		"DEFAULT STARTS"	startdefault,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NETWORK INACTIVE"	inactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			relogin;

sendpasswd:
	crc val npasswd;
	write "PASSWORD " npasswd val "\r";
	timeout 10;
	expect
		" error -- "		shellerr,
		" ERROR -- "		shellerr,
		"[Nn]o directory"	shellerr,
		"NO DIRECTORY"		shellerr,
		"FAILED PASSWORD"	failpasswd,
		"QUERY PASSWORD"	sendpasswd,
		"SHELL STARTS"		params,
		"DAEMON STARTS"		startdaemon,
		"DEFAULT STARTS"	startdefault,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NETWORK INACTIVE"	inactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			relogin;

params:
	test paramcount			startdefault;
	crc val remotedmnargs;
	write "PARAMS " remotedmnargs val "\r";
	timeout 10;
	expect
		"SHELL STARTS"		params,
		"DEFAULT STARTS"	startdefault,
		"DAEMON STARTS"		startdaemon,
		"DAEMON ALREADY ACTIVE"	alreadyactive,
		"NO CARRIER"		clrmodem,
		EOF			eof,
		TERMINATE		terminate,
		TIMEOUT			params;

startdefault:
	trace "default parameters used";
	set localdmnargs "-cB5 -D16 -R300 -S10 -X30";
startdaemon:
	forkdaemon localdmnargs;		/* Transport daemon exits and returns here */
endmodem:
	device "local";				/* Ignore modem signals */
	sleep 2;
	device "flush";
	slowwrite "+++";
	timeout 3;
	expect
		"NO CARRIER"		endmodem1,
		"OK"			endmodem1,
		"ERROR"			modem_err,
		EOF			endmodem2,
		TERMINATE		endmodem0,
		TIMEOUT			endmodem1;

endmodem0:
	timeout 0;
	sleep 2;
	device "flush";
endmodem1:
	timeout 0;
	sleep 1;
	slowwrite "ATH0\r";
	timeout 2;
	expect
		"OK"			endmodem2,
		"ERROR"			modem_err,
		EOF			endmodem2,
		TERMINATE		endmodem2,
		TIMEOUT			endmodem2;

endmodem2:
	timeout 0;
	match finishstr ".+"		endmodem3;
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
	next				endmodem;

dfail:
	match dialstr2 UNDEFINED	dfail2;
	trace "number \"" dialstr "\" busy, trying \"" dialstr2 "\"";
	next				nextnumber;
dfail2:	
	set reason "too many dial attempts";
	next				endmodem;

eof:
	set reason "remote disconnect";
	next				endmodem;

failpasswd:
	set reason "password incorrect";
	next				endmodem;

failperm:
	set reason "promiscuous connection denied";
	next				endmodem;

inactive:
	set reason "remote network inactive";
	next				endmodem;

lfail:
	match dialstr2 UNDEFINED	lfail2;
	trace "number \"" dialstr "\" has no getty, trying \"" dialstr2 "\"";
	set dialstr dialstr2;
	set dialstr2 UNDEFINED;
	set dialcount 3;
	next				rstmodem;
lfail2:
	set reason "too many login attempts";
	next				endmodem;

nodial:
	set reason "modem \"" modemdev "\" has no dialtone";
	match modemdev2 UNDEFINED endmodem;
	next				nextmodem;

rfail:
	set reason "login timed-out";
	next				endmodem;

shellerr:
	set reason "remote: " INPUT;
	next				endmodem;

terminate:
	set reason "system termination";
	next				endmodem;
