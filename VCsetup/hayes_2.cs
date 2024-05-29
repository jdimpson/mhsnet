/*
**	Call script to dial a remote site (called by "hayes_1.cs").
**
**	This script calls "hayes_3.cs" to get login prompt.
**
**	Imports:
**		[cook]		parameter to turn on circuit cooking ("-c")
**		dialstr		string to cause modem to dial remote site (<number>@<speed>)
**		[dmnargs]	extra params for daemon
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		[fixedspd]	"true" for fixed-speed modem connection
**		[initstr]	initial AT commands for modem
**		loginstr	login name at remote site
**		[longterm]	seconds for slow terminate
**		[myscript]	optional script run after modem connects, before unix login
**		[noisy]		"true" for noisy lines: small packets, low throughput
**		[packettrace]	local packet trace level
**		passwdstr	password for login name at remote site
**		[permanent]	"true" for non-batch mode connections
**		[priority]	max. message priority transmitted
**		[sun3]		expect SUN III daemon
**		type		modem type
**
**	Modem types supported:
**		avtek
**		fastbit2
**		hayes
**		interlink
**		netcomm
**			(any of above +XON for COOK mode with XON/XOFF flow-control)
**		interlinkRTS	RS-232 flow-control	(recommended!)
**		trailblazerRTS	RS-232 flow-control	(recommended!)
**		trailblazerXON	XON/XOFF flow-control
*/

	set dialcount	 3;			/* dial attempts */

	match type				/* Set initstr / finishstr */
		"trailblazerRTS"	_2_settbRTS,
		"trailblazerXON"	_2_settbXON,
		"netcomm"		_2_setnetcomm,
		"interlinkRTS"		_2_setinterlinkRTS,
		"interlink"		_2_setinterlink,
		"fastbit2"		_2_setfastbit2,
		"avtek"			_2_setavtek;

_2_start:
	shift dial "@" dialstr;			/* Have number@speed */
	match dial UNDEFINED	_2_paramerr2;
	match dialstr UNDEFINED	_2_paramerr1;
	set speed dialstr;
	match ether_ts "."	_2_start2;
_2_start0:
	device "speed" speed;
	match fixedspd UNDEFINED _2_start1;
	match fixedspd "true"	_2_start2;
_2_start1:
	set fixedspd "false";
_2_start2:

	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite "ATE0Q0V1S0=0S2=43\r";
	timeout 10;
	expect
		"OK"		_2_try_initstr,
		"ERROR"		_2_modem_err,
		EOF		_2_eof,
		TERMINATE	_2_terminate,
		TIMEOUT		_2_rstmodem;

_2_rstmodem:
	device "local";
	sleep 2;
	device "flush";
	slowwrite "+++";
	timeout 10;
	expect
		"OK"		_2_clrmodem,
		"NO CARRIER"	_2_clrmodem,
		"ERROR"		_2_modem_err,
		EOF		_2_eof,
		TERMINATE	_2_terminate,
		TIMEOUT		_2_clrmodem;
_2_clrmodem:
	sleep 2;
	slowwrite "ATE0Q0V1S0=0S2=43H0\r";
	timeout 10;
	expect
		"OK"		_2_try_initstr,
		"NO CARRIER"	_2_try_initstr,
		"ERROR"		_2_modem_err,
		EOF		_2_eof,
		TERMINATE	_2_terminate,
		TIMEOUT		_2_nomodem;

_2_check_initstr:
	timeout 10;
	expect
		"OK"		_2_try_initstr,
		"ERROR"		_2_modem_err,
		EOF		_2_eof,
		TERMINATE	_2_terminate,
		TIMEOUT		_2_try_initstr;

_2_try_initstr:
	shift nextstr " " initstr;
	match nextstr UNDEFINED	_2_dialing;
	match nextstr "Q1"	_2_try_initstr;	/* Disallow quiet mode */
	match nextstr "&Q0"	_2_try_initstr;
	slowwrite "AT" nextstr "\r";
	next			_2_check_initstr;

_2_slp_initstr:
	sleep 1;
	next			_2_try_initstr;

_2_dialing:
	test dialcount		_2_busy;
	timeout 0;
	sleep 1;
	slowwrite "ATD" dial "\r";
	timeout 60;
	expect
		"BUSY"		_2_busy,
		"NO ANSWER"	_2_noanswer,
		"NO CARRIER"	_2_clrmodem,
		"NO DIALTONE"	_2_nodial,
		"CONNECT 28800"	_2_got28800,
		"CONNECT 26400"	_2_got28800,
		"CONNECT 24000"	_2_got28800,
		"CONNECT 21600"	_2_got28800,
		"CONNECT 19200"	_2_got28800,
		"CONNECT 16800"	_2_got28800,
		"CONNECT FAST"	_2_got14400,
		"CONNECT 14K4"	_2_got14400,
		"CONNECT 12K"	_2_got12000,
		"CONNECT 14400"	_2_got14400,
		"CONNECT 12000"	_2_got12000,
		"CONNECT 9600"	_2_got9600,
		"CONNECT 4800"	_2_got4800,
		"CONNECT 2400"	_2_got2400,
		"CONNECT 1200"	_2_got1200,
		"CONNECT 600"	_2_got600,
		"CONNECT 300"	_2_got300,
		"CONNECT"	_2_gotdefault,
		"ERROR"		_2_modem_err,
		EOF		_2_eof,
		TERMINATE	_2_terminate,
		TIMEOUT		_2_rstmodem;

_2_trylogin:
	match type
		"trailblazer"	_2_trylogin2,	/* TB settings added on speed match */
		"XON"		_2_setcook,	/* Turn on cooked mode */
		"RTS"		_2_setrts;	/* Turn on tty RTS/CTS mode */
_2_trylogin2:
	match permanent "true"	_2_setdod;	/* Set `die-on-down' */
	match longterm UNDEFINED _2_setshort;
	set localdmnargs "-B" longterm " " localdmnargs;
	set remotedmnargs "-B" longterm " " remotedmnargs;
_2_trylogin3:
	match priority UNDEFINED _2_trylogin34;
	match priority "-Q9"	_2_trylogin34;	/* Ignore default */
	set localdmnargs priority " " localdmnargs;
	set remotedmnargs priority " " remotedmnargs;
_2_trylogin34:
	match packettrace UNDEFINED _2_trylogin4;
	match packettrace "-P0"	_2_trylogin4;	/* Ignore default */
	set localdmnargs packettrace " " localdmnargs;	/* Only local */
_2_trylogin4:
	match cook UNDEFINED	_2_trylogin5;
	match localdmnargs "-c " _2_trylogin5;
	match localdmnargs "-CX " _2_trylogin5;
	set localdmnargs cook " " localdmnargs;
	set remotedmnargs cook " " remotedmnargs;
_2_trylogin5:
	match dmnargs UNDEFINED	_2_trylogin6;
	set localdmnargs localdmnargs " " dmnargs;
	set remotedmnargs remotedmnargs " " dmnargs;
_2_trylogin6:
	match myscript UNDEFINED _2_trylogin7;
	call myscript;				/* Own script */
	match reason "."	_2_endmodem;	/* Error */
_2_trylogin7:
	call "_call/hayes_3.cs";		/* Attempt login */
	match reason "remote disconnect" _2_endmodem5;
	match reason "NO CARRIER" _2_endmodem3;
_2_endmodem:
	set modemcount		3;		/* Try +++ twice */
	match ether_ts "."	_2_endmodem_0;
	device "local";				/* Ignore modem signals */
_2_endmodem_0:
	test modemcount		_2_endmodem1;
	sleep 2;
	device "flush";
	slowwrite "+++";
	timeout 3;
	expect
		"NO CARRIER"	_2_endmodem1,
		"OK"		_2_endmodem1,
		"ERROR"		_2_modem_err,
		EOF		_2_endmodem5,
		TERMINATE	_2_endmodem0,
		TIMEOUT		_2_endmodem_0;

_2_endmodem0:
	set terminate "true";
	set reason "system termination";
	timeout 0;
	sleep 2;
	device "flush";
_2_endmodem1:
	timeout 0;
	sleep 1;
	slowwrite "ATH0\r";
	timeout 4;
	expect
		"OK"		_2_endmodem3,
		"ERROR"		_2_modem_err,
		EOF		_2_endmodem5,
		TERMINATE	_2_endmodem2,
		TIMEOUT		_2_endmodem3;	/* Send finishstr anyway */

_2_endmodem2:
	set terminate "true";
	set reason "system termination";
_2_endmodem3:
	timeout 0;
	match finishstr ".+"	_2_endmodem4;
	set finishstr "ATE0Q1S0=2";
_2_endmodem4:
	slowwrite finishstr "\r";
_2_endmodem5:
	return;

_2_setshort:
	set localdmnargs "-B10 " localdmnargs;
	set remotedmnargs "-B10 " remotedmnargs;
	next			_2_trylogin3;

_2_setdod:
	set localdmnargs "-d " localdmnargs;
	set remotedmnargs "-d " remotedmnargs;
	next			_2_trylogin3;

_2_gotdefault:
	match speed
		"EXTB"		_2_got38400,
		"38400"		_2_got38400,
		"EXTA"		_2_got19200,
		"19200"		_2_got19200,
		"9600"		_2_got9600,
		"4800"		_2_got4800,
		"2400"		_2_got2400,
		"1200"		_2_got1200,
		"600"		_2_got600,
		"300"		_2_got300,
		".*"		_2_badspeed;

_2_got38400:
	match fixedspd "true"	_2_got19200ns;
	set speed "38400";
	device "speed" speed;
	next			_2_got19200ns;

_2_got28800:
	match fixedspd "true"	_2_got28800ns;
	set speed "38400";
	device "speed" speed;
_2_got28800ns:
	match type
		"hayes"		_2_hayes288,
		"trailblazerRTS" _2_tbRTS192,
		"trailblazerXON" _2_tbXON192,
		".*"		_2_badtype;

_2_got19200:
	match fixedspd "true"	_2_got19200ns;
	set speed "19200";
	device "speed" speed;
_2_got19200ns:
	match type
		"hayes"		_2_hayes192,
		"trailblazerRTS" _2_tbRTS192,
		"trailblazerXON" _2_tbXON192,
		".*"		_2_badtype;

_2_got14400:
	match fixedspd "true"	_2_got14400ns;
	set speed "19200";
	device "speed" speed;
_2_got14400ns:
	match type
		"hayes"		_2_hayes192,
		"trailblazerRTS" _2_tbRTS144,
		"trailblazerXON" _2_tbXON144,
		".*"		_2_badtype;

_2_got12000:
	match fixedspd "true"	_2_got12000ns;
	set speed "19200";
	device "speed" speed;
_2_got12000ns:
	match type
		"hayes"		_2_hayes120,
		"trailblazerRTS" _2_tbRTS120,
		"trailblazerXON" _2_tbXON120,
		".*"		_2_badtype;

_2_got9600:
	match fixedspd "true"	_2_got9600ns;
	set speed "9600";
	device "speed" speed;
_2_got9600ns:
	match type
		"hayes"		_2_hayes96,
		"trailblazer"	_2_hayes96,
		".*"		_2_badtype;

_2_got4800:
	match fixedspd "true"	_2_hayes48;
	set speed "4800";
	device "speed" speed;
	next			_2_hayes48;

_2_got2400:
	match fixedspd "true"	_2_hayes24;
	set speed "2400";
	device "speed" speed;
	next			_2_hayes24;

_2_got1200:
	match fixedspd "true"	_2_hayes12;
	set speed "1200";
	device "speed" speed;
	next			_2_hayes12;

_2_got600:
	match fixedspd "true"	_2_hayes6;
	set speed "600";
	device "speed" speed;
	next			_2_hayes6;

_2_got300:
	match fixedspd "true"	_2_hayes3;
	set speed "300";
	device "speed" speed;
	next			_2_hayes3;

_2_hayes288:
	match noisy "true"	_2_hayes192n;
	set localdmnargs "-D1024 -S200 -X2000";
	set remotedmnargs "-D1024 -S200 -X2000";
	next			_2_trylogin;
_2_hayes192:
	match noisy "true"	_2_hayes48n;
_2_hayes192n:
	set localdmnargs "-D256 -S200 -X1380";
	set remotedmnargs "-D256 -S200 -X1380";
	next			_2_trylogin;
_2_hayes120:
	match noisy "true"	_2_hayes48n;
	set localdmnargs "-D256 -S200 -X1100";
	set remotedmnargs "-D256 -S200 -X1100";
	next			_2_trylogin;
_2_hayes96:
	match noisy "true"	_2_hayes24n;
	set localdmnargs "-D256 -S200 -X960";
	set remotedmnargs "-D256 -S200 -X960";
	next			_2_trylogin;
_2_hayes48:
	match noisy "true"	_2_hayes12n;
_2_hayes48n:
	set localdmnargs "-D128 -S100 -X480";
	set remotedmnargs "-D128 -S100 -X480";
	next			_2_trylogin;
_2_hayes24:
	match noisy "true"	_2_hayes12n;
_2_hayes24n:
	set localdmnargs "-D64 -S50 -X240";
	set remotedmnargs "-D64 -S50 -X240";
	next			_2_trylogin;
_2_hayes12:
	match noisy "true"	_2_hayes6n;
_2_hayes12n:
	set localdmnargs "-D32 -S20 -X120";
	set remotedmnargs "-D32 -S20 -X120";
	next			_2_trylogin;
_2_hayes6:
	match noisy "true"	_2_hayes3n;
_2_hayes6n:
	set localdmnargs "-D16 -S10 -X60";
	set remotedmnargs "-D16 -S10 -X60";
	next			_2_trylogin;
_2_hayes3:
	match noisy "true"	_2_hayes1;
_2_hayes3n:
	set localdmnargs "-D16 -S5 -X30";
	set remotedmnargs "-D16 -S5 -X30";
	next			_2_trylogin;
_2_hayes1:
	set localdmnargs "-D8 -S3 -X30";
	set remotedmnargs "-D8 -S3 -X30";
	next			_2_trylogin;

_2_tbRTS192:
	match noisy "true"	_2_tbRTS192n;
	set localdmnargs "-notD1024 -S200 -X1380";
	set remotedmnargs "-nitD1024 -S200 -X1380";
	next			_2_trylogin;
_2_tbRTS192n:
	set localdmnargs "-notD256 -S100 -X1380";
	set remotedmnargs "-nitD256 -S100 -X1380";
	next			_2_trylogin;
_2_tbRTS144:
	match noisy "true"	_2_tbRTS120n;
	set localdmnargs "-D1024 -S200 -X1380";
	set remotedmnargs "-D1024 -S200 -X1380";
	next			_2_trylogin;
_2_tbRTS120:
	match noisy "true"	_2_tbRTS120n;
	set localdmnargs "-D1024 -S200 -X1100";
	set remotedmnargs "-D1024 -S200 -X1100";
	next			_2_trylogin;
_2_tbRTS120n:
	set localdmnargs "-D128 -S100 -X1100";
	set remotedmnargs "-D128 -S100 -X1100";
	next			_2_trylogin;

_2_tbXON192:
	match noisy "true"	_2_tbXON192n;
	set localdmnargs "-CX -notD1024 -S200 -X1380";
	set remotedmnargs "-CX -nitD1024 -S200 -X1380";
	next			_2_trylogin;
_2_tbXON192n:
	set localdmnargs "-CX -notD256 -S100 -X1380";
	set remotedmnargs "-CX -nitD256 -S100 -X1380";
	next			_2_trylogin;
_2_tbXON144:
	match noisy "true"	_2_tbXON120n;
	set localdmnargs "-CX -D1024 -S200 -X1380";
	set remotedmnargs "-CX -D1024 -S200 -X1380";
	next			_2_trylogin;
_2_tbXON120:
	match noisy "true"	_2_tbXON120n;
	set localdmnargs "-CX -D1024 -S200 -X1100";
	set remotedmnargs "-CX -D1024 -S200 -X1100";
	next			_2_trylogin;
_2_tbXON120n:
	set localdmnargs "-CX -D128 -S100 -X1100";
	set remotedmnargs "-CX -D128 -S100 -X1100";
	next			_2_trylogin;

_2_setcook:
	match cook ".*"		_2_trylogin2;
	match localdmnargs
		"-c "		_2_trylogin2,
		"-CX "		_2_trylogin2;
	set localdmnargs "-CX " localdmnargs;
	set remotedmnargs "-CX " remotedmnargs;
	next			_2_trylogin2;

_2_setrts:
	match VERSION
		"V.4"		_2_setrts_V42,
		"SunOS.5"	_2_setrts_V42,
		"XENIX"		_2_setrts_xenix,
		"BSDI"		_2_setrts_bsdi;
	trace "delete one of the following, or add your own RTS/CTS setup here ...";
	device "stty" "crtscts";
	match RESULT DEVOK	_2_trylogin2;
	device "stty" "rtscts";
	match RESULT DEVOK	_2_trylogin2;
_2_setrts_error:
	trace "stty to set RTS/CTS flow-control failed";
	next			_2_trylogin2;

_2_setrts_V42:
	device "stty" "rtsxoff ctsxon";
	match RESULT DEVOK	_2_trylogin2;
	next			_2_setrts_error;

_2_setrts_bsdi:
	device "stty" "rts_iflow cts_oflow";
	match RESULT DEVOK	_2_trylogin2;
	next			_2_setrts_error;

_2_setrts_xenix:
	device "stty" "rtsflow ctsflow";
	match RESULT DEVOK	_2_trylogin2;
	next			_2_setrts_error;

/*
**	V1	return responses in english
**	X0	only return very simple CONNECT messages (no baud rate)
**	Y0	disable long space disconnect
**	S51=5	connection from modem to computer fixed at 19200 baud
**	S52=1	modem hangs up when DTR goes from on to off, won't answer if DTR is off
**	S54=2	send breaks to remote modem
**	S58=2	use RTS/CTS flow control from modem to computer
**	S66=1	lock interface speed to computer and use flow control in low speed modes
**	S68=2	use RTS/CTS flow control from computer to modem
**	S90=1	use CCITT standards for connecting at 300 or 1200 baud
**	S110=1	enable data compression when in PEP mode
**	S111=0	disable all file transfer procotols
**	S255=0	use the profile selected by the A/B switch on the console
*/
_2_settbRTS:
	match initstr UNDEFINED	_2_settbRTS1;
	next			_2_start;
_2_settbRTS1:
	set initstr "S54=2S58=2S66=1S67=0S68=2S110=1S111=0S255=0 X0V1 ";
	set finishstr "ATZ";
	next			_2_start;

/*
**	S58=3	use XON/XOFF flow control from modem to computer
**	S68=3	use XON/XOFF flow control from computer to modem
*/
_2_settbXON:
	match initstr UNDEFINED	_2_settbXON1;
	next			_2_start;
_2_settbXON1:
	set initstr "S54=2S58=3S66=1S67=0S68=3S110=1S111=0S255=0 X0V1 ";
	set finishstr "ATZ";
	next			_2_start;

_2_setfastbit2:
	match initstr UNDEFINED	_2_setfastbit21;
	next			_2_start;
_2_setfastbit21:
	set initstr "$E0B14&D0 ";
	set finishstr "ATZ";
_2_sethayes:
	match type
		"XON"		_2_sethayesXON,
		"RTS"		_2_sethayesRTS;
	set type "hayes";
	next			_2_start;
_2_sethayesXON:
	set type "hayesXON";
	next			_2_start;
_2_sethayesRTS:
	set type "hayesRTS";
	next			_2_start;

/*
**	&E2	auto-reliable - use MNP if available
*/
_2_setavtek:
	match initstr UNDEFINED	_2_setavtek1;
	next			_2_sethayes;
_2_setavtek1:
	set initstr "V1 &E2";
	set finishstr "ATZ";
	next			_2_sethayes;

_2_setnetcomm:
	match initstr UNDEFINED	_2_setnetcomm1;
	next			_2_sethayes;
_2_setnetcomm1:
	set initstr "V1X1M1B2 &A1 ";
	set finishstr "ATZ";
	next			_2_sethayes;

/*
**	X2	send result codes 0-6 and 8-15, and wait for dial tone
**	$S1	set baud rate between modem and computer to telecom line baud rate
**	$E0	turn off MNP
**	&D2	enter command mode and hang up phone when DTR drops
*/
_2_setinterlink:
	match initstr UNDEFINED	_2_setinterlink1;
	next			_2_sethayes;
_2_setinterlink1:
	set initstr "X2$S1$E0&D2 ";
	next			_2_sethayes;

/*
**	X0	blind dial, only send result codes 0-4 (don't send baud rate)
**	X6	no blind dial, issue all result codes
**	$C0	disable MNP5 (compression)
**	$E1	enable MNP4 (20% faster) negotiation
**	$S7	lock baud rate between computer and modem at 19200
*/
_2_setinterlinkRTS:
	match initstr UNDEFINED	_2_setinterlinkRTS1;
	next			_2_sethayes;
_2_setinterlinkRTS1:
	set initstr "X6$E1$C0&D2 ";
	next			_2_sethayes;

_2_badspeed:
	fail "hayes_2.cs: unrecognised speed: " speed;
	return;

_2_badtype:
	fail "hayes_2.cs: unrecognised modem type: " type;
	return;

_2_busy:
	set reason "BUSY";
	next			_2_endmodem3;

_2_eof:
	match RESULT UNDEFINED	_2_eof1,
			DEVOK	_2_eof1;
	trace "read error: " RESULT;
_2_eof1:
	set reason "EOF from modem";
	next			_2_endmodem5;

_2_modem_err:
	set reason "modem command error";
	return;

_2_noanswer:
	set reason "NO ANSWER";
	next			_2_endmodem3;

_2_nomodem:
	set reason "modem not responding";
	return;

_2_nodial:
	set reason "modem has no dialtone";
	next			_2_endmodem3;

_2_paramerr1:
	set dialstr dial;
_2_paramerr2:
	fail "\"dialstrings\" missing \"@\": " dialstr;
	return;

_2_terminate:
	set terminate "true";
	set reason "system termination";
	next			_2_endmodem;
