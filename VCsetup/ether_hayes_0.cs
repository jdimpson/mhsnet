/*
**	Call script to call a host via a Hayes compatible modem
**	attached to an ethernet terminal server (called by "call.ether_modets_0").
**
**	This script calls "ether_hayes_1.cs" once for each modem specified.
**
**	Imports:
**		[cook]		parameter to turn on circuit cooking ("-c")
**		dialstrings	string(s) to cause modem to dial remote site (<number>@<speed>[|...])
**		[dmnargs]	extra params for daemon
**		[dmnname]	alternate transport daemon [default: VCdaemon]
**		ether_ts	name or IP number for ethernet terminal server
**		ets_username	ethernet terminal server user validation
**		ets_password	ethernet terminal server password validation
**		[fixedspd]	"true" for fixed-speed modem connection
**		[initstr]	initial AT commands for modem
**		loginstr	login name at remote site
**		[longterm]	optional seconds for slow terminate [default: 5]
**		modems		modem(s) attached to device(s) (<type>@<port>[|...])
**		[myscript]	optional script run after modem connects, before unix login
**		[noisy]		"true" for noisy lines: small packets, low throughput
**		[packettrace]	local packet trace level
**		[passwdstr]	password for login name at remote site [default: null]
**		[permanent]	"true" for non-batch mode connections
**		[priority]	max. message priority transmitted
**		[sun3]		expect SUN III daemon
*/

	monitor 3;				/* turn on I/O + callscript tracing */

	set opentimeout	60;			/* Wait 1 minute between/for open attempts */
	set openretries	60;			/* Allow 60 attempts at open => 1 hour max */

	set fixedspd "true";

	/*
	**	Check imported strings.
	*/

	match ether_ts UNDEFINED	_0_paramerr;
	match ets_username UNDEFINED	_0_paramerr;
	match ets_password UNDEFINED	_0_paramerr;
	match dialstrings UNDEFINED	_0_paramerr;
	match loginstr UNDEFINED	_0_paramerr;
	match modems UNDEFINED		_0_paramerr;
	match passwdstr UNDEFINED	_0_setpw;
_0_nextmodem:						/* MAIN LOOP STARTS */
	shift modem "|" modems;
	match modem UNDEFINED		_0_openfail;
	shift type "@" modem;			/* Have type@port */
	match type UNDEFINED		_0_paramerratt2;
	match modem UNDEFINED		_0_paramerratt1;
	set dialset dialstrings;		/* Now set for ether_hayes_1 */
	call "_call/ether_hayes_1.cs";		/* Connect to server / open modem */
	match terminate "true"		_0_finish;
	match reason UNDEFINED		_0_nextmodem;
	trace reason;
	next				_0_nextmodem;	/* MAIN LOOP ENDS */

_0_finish:
	match reason UNDEFINED		_0_finishok;
	fail reason;
_0_finishok:
	return;

_0_setpw:
	set passwdstr "";			/* "passwdstr" defaults to null password */
	next				_0_nextmodem;

_0_paramerr:
	fail "missing some of:\n"
		"\t-D \"ether_ts=<ethernet terminal server name>\"\n"
		"\t-D \"ets_username=<ethernet terminal server user>\"\n"
		"\t-D \"ets_password=<ethernet terminal server password>\"\n"
		"\t-D \"dialstrings=<telno>@<bits/second>[|...]\"\n"
		"\t-D \"loginstr=<login name at remote site>\"\n"
		"\t-D \"modems=<modem type>@<ether_ts port>[|...]\""
		;

_0_paramerratt1:
	set modem type;
_0_paramerratt2:
	fail "\"modems\" missing \"@\": " modem;

_0_openfail:
	match reason UNDEFINED		_0_finishok;
	fail reason;
