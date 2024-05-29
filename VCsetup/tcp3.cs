/*
**	Call script to login at a SUNIII host connected via a TCP/IP link.
**
**	Imports:
**		[dmnargs]	alternate for localdmnargs
**		[localdmnargs]	parameters for daemon at calling site
**		server		remote network server name (eg: mhsnet)
**		target		remote node name
**		[port]		optional IP port number if no ``/etc/services''
*/

	monitor 2;				/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;
chkargs:
	match target UNDEFINED paramerr;
	match server UNDEFINED paramerr;
start:
	timeout 60;
	match port UNDEFINED noport;
	open "tcp" target server port;
	next checkopen;
noport:
	open "tcp" target server;
checkopen:
	match RESULT DEVOK openok;
	fail "Could not connect to " target ", reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"target=<IP name of remote site>\"\n"
		"\t-D \"server=<server name for port, eg: mhsnet>\""
		;

openok:
	expect
		" error -- " shellerr,
		"[Nn]o directory" shellerr,
		"You are unknown to me" shellerr,
		"Unrecognised shell" shellerr,
		"PNdaemon starts" startPNdaemon,
		"ENdaemon starts" startNNdaemon,
		"daemon starts" startNNdaemon,
		"daemon already active" alreadyactive,
		EOF eof,
		TERMINATE terminate,
		TIMEOUT timedout;

startdaemon:
/*	trace "connection successful";	*/
	execdaemon localdmnargs;		/* No return */
	return;

alreadyactive:
	fail "remote daemon already active";

eof:
	fail "remote disconnect";

timedout:
	fail "connection timed-out";

shellerr:
	fail "remote " INPUT;

terminate:
	fail "system termination";

startNNdaemon:
	daemon "NNdaemon";
	next startdaemon;

startPNdaemon:
	daemon "PNdaemon";
	next startdaemon;
