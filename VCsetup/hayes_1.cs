/*
**	Call script to open a modem for dialling (called by "hayes_0.cs").
**
**	This script calls "hayes_2.cs" once for each dialstring specified.
**
**	Imports:
**		[cook]		parameter to turn on circuit cooking ("-c")
**		dialset		string(s) to cause modem to dial remote site (<number>@<speed>[|...])
**		[dmnargs]	extra params for daemon
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		[fixedspd]	"true" for fixed-speed modem connection
**		[initstr]	initial AT commands for modem
**		[longterm]	seconds for slow terminate
**		loginstr	login name at remote site
**		modem		device attached to modem
**		[myscript]	optional script run after modem connects, before unix login
**		[noisy]		"true" for noisy lines: small packets, low throughput
**		opentimeout	TIMEOUT for open
**		openretries	number of open attempts
**		[packettrace]	local packet trace level
**		passwdstr	password for login name at remote site
**		[permanent]	"true" for non-batch mode connections
**		[priority]	max. message priority transmitted
**		[sun3]		expect SUN III daemon
**		type		modem type
*/

	timeout opentimeout;
	retry openretries;
	open "dial" modem "uucplock" "ondelay" "local";	/* Open with O_NDELAY */
	match RESULT DEVOK	_1_openok;
	set reason "Could not open \"" modem "\" reason: " RESULT;
_1_nextmdm:
	close;					/* Close any open device */
	return;					/* Try next modem */

_1_openok:
	device "offdelay";			/* Otherwise reads will return 0 */
	match VERSION "BSDI"	_1_openbsdi;
	next			_1_openok2;
_1_openbsdi:
	device "stty" "clocal";			/* Ensure CLOCAL is on. BSDI ONLY  */
	match RESULT DEVOK	_1_openok2;	/* BSDI ONLY */
	trace "Set CLOCAL FAILED";		/* BSDI ONLY */
_1_openok2:
	match type "XON"	_1_doxon;
_1_nextdial:					/* MAIN LOOP STARTS */
	shift dialstr "|" dialset;
	match dialstr UNDEFINED	_1_nextmdm;
	set reason UNDEFINED;
	set savdial dialstr;
	call "_call/hayes_2.cs";		/* Attempt connection */
	match terminate "true"	_1_nextmdm;
	match reason UNDEFINED	_1_nextdial;
	match reason "modem"	_1_nextmdm;	/* Modem error */
	set reason "telno \"" savdial "\" error: " reason;
	trace reason;
	next			_1_nextdial;	/* MAIN LOOP ENDS */

_1_doxon:
	device "xonxoff";
	next			_1_nextdial;
