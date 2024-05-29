/*
**	Call script to test a Hayes compatible modem.
**
**	Imports:
**		linespeed	speed for modemdev
**		modemdev	device attached to modem
*/

	monitor 20;		/* Turn on ALL tracing */
	set eofcount 3;

	/*
	**	Check imported strings.
	*/

	match linespeed UNDEFINED paramerr;
	match modemdev UNDEFINED paramerr;

	timeout 10;				/* Wait 10 seconds between/for open attempts */
	retry 3;				/* Allow 3 attempts at open => 30 secs. max */
	open "dial" modemdev "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK openok;
	fail "Could not open \"" modemdev "\" reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"modemdev=<device pathname for modem>\""
		;

openok:
	device "offdelay";			/* Otherwise reads will return 0 */
	match RESULT DEVOK openok2;
	fail "Could not turn \"offdelay\" reason: " RESULT;
openok2:
	device "speed" linespeed;
	match RESULT DEVOK openok3;
	fail "Could not set speed \"" linespeed "\" reason: " RESULT;
openok3:
/*	shell "stty everything; exit 1";	/* Produce a dump of the modes */
	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite "ATE0Q0S0=0S2=43\r";
	timeout 10;
	expect
		"OK"		setup,
		"ERROR"		modem_err,
		EOF		eof,
		TERMINATE	terminate,
		TIMEOUT		rstmodem;

rstmodem:
	sleep 2;
	write "+++";
	sleep 2;
	slowwrite "ATE0Q0S0=0S2=43H0\r";
	timeout 10;
	expect
		"OK"		setup,
		"ERROR"		modem_err,
		EOF		eof,
		TERMINATE	terminate,
		TIMEOUT		nomodem;

setup:
	close;
	sleep 10;				/* Let things settle down */
	timeout 10;				/* Wait 10 seconds between/for open attempts */
	retry 3;				/* Allow 3 attempts at open => 30 secs. max */
	open "dial" modemdev "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK openok10;
	fail "Could not open \"" modemdev "\" reason: " RESULT;

openok10:
	device "offdelay";			/* Otherwise reads will return 0 */
	match RESULT DEVOK openok20;
	fail "Could not turn \"offdelay\" reason: " RESULT;
openok20:
	device "speed" linespeed;
	match RESULT DEVOK openok30;
	fail "Could not set speed \"" linespeed "\" reason: " RESULT;
openok30:
	sleep 1;
/*	shell "stty everything; exit 1";	/* */
again:	sleep 1;				/* Because some terminal i/o is slow! */
	slowwrite "ATE0Q0S0=1\r";
	timeout 10;
	expect
		"OK"		end,
		"ERROR"		modem_err,
		EOF		eof1,
		TERMINATE	terminate,
		TIMEOUT		rstmodem2;

rstmodem2:
	sleep 2;
	write "+++";
	sleep 2;
	slowwrite "ATE0Q0S0=1H0\r";
	timeout 10;
	expect
		"OK"		end,
		"ERROR"		modem_err,
		EOF		eof,
		TERMINATE	terminate,
		TIMEOUT		nomodem;

eof1:
	test eofcount eof;
	match RESULT UNDEFINED eof10;
	trace "EOF from modem, reason: " RESULT;
	next again;
eof10:
	trace "EOF from modem";
	next again;

eof:
	match RESULT UNDEFINED eof2;
	fail "EOF from modem, reason: " RESULT;
eof2:
	fail "EOF from modem";

modem_err:
	fail "modem command error";

nomodem:
	fail "modem \"" modemdev "\" not responding";

terminate:
	fail "system termination";

end:
	slowwrite "ATQ1\r";
	return;
