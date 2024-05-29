/*
**	Call script to program a Hayes compatible modem.
**
**	Imports:
**		initstr		initialisation for modem
**				[possibly separated by spaces]
**		linespeed	speed for modemdev
**		modemdev	device attached to modem
**		[axuser]	optional user for annex
**		[axpasswd]	optional passwd for annex
*/

	monitor 2;		/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match initstr UNDEFINED paramerr;
	match linespeed UNDEFINED paramerr;
	match modemdev UNDEFINED paramerr;

	timeout 10;				/* Wait 10 seconds between/for open attempts */
	retry 3;				/* Allow 3 attempts at open => 30 secs. max */
	open "dial" modemdev "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK openok;
	fail "Could not open \"" modemdev "\" reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"initstr=<AT initialisation command for modem>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"modemdev=<device pathname for modem>\""
		;

axparamerr:
	fail "missing some of:\n"
		"\t-D \"axuser=<Annex user>\"\n"
		"\t-D \"axpasswd=<Annex password>\""
		;

openok:
	device "offdelay";			/* Otherwise reads will return 0 */
	device "speed" linespeed;
	sleep 1;				/* Because some terminal i/o is slow! */
	match axuser UNDEFINED direct;
	write "\r";
	timeout 3;
	expect
		"Permission granted" direct,
		"Annex username:" ax_1,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT	direct;

ax_1:
	write axuser "\r";
	expect
		"Permission granted" direct,
		"Annex password:" ax_2,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT	direct;

ax_2:
	match axpasswd UNDEFINED axparamerr;
	write axpasswd "\r";
	expect
		"Permission granted" direct,
		"Annex password:" ax_2,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT	direct;

direct:
	trace "Change escape code to ~~~ with 2 second guard";
	trace "to prevent remote users accidentally re-programming our modems";
	device "flush";
	slowwrite "ATE0Q0V1S0=0S2=126S12=100\r";
	timeout 10;
	expect
		"OK" setup,
		"ERROR" error,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT rstmodem;

rstmodem:
	sleep 3;
	slowwrite "~~~";
	sleep 3;
	device "flush";
	slowwrite "ATE0Q0V1S0=0S2=126S12=100\r";
	timeout 10;
	expect
		"OK" setup,
		"ERROR" error,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT rstmodem2;

rstmodem2:
	sleep 3;
	slowwrite "+++";
	sleep 3;
	device "flush";
	slowwrite "ATE0Q0V1S0=0S2=126S12=100H0\r";
check:
	timeout 10;
	expect
		"OK" setup,
		"ERROR" error,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT nomodem;

setup:
	shift nextstr " " initstr;
	match nextstr UNDEFINED end;
	slowwrite "AT" nextstr "\r";
	match nextstr "Q1" slsetup;
	match nextstr "&Q0" slsetup;
	next check;

slsetup:
	sleep 1;
	next setup;

eof:
	fail "EOF from modem";

error:
	fail "ERROR from modem";

nomodem:
	fail "modem \"" modemdev "\" not responding";

terminate:
	fail "system termination";

end:
	return;
