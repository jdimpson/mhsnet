/*
**	Call script to connect to a SUNIII host via an UDP/IP network.
**
**	Imports:
**		[dmnargs]	== localdmnargs
**		[localdmnargs]	parameters for daemon at calling site
**		server		remote network server name (eg: acsnet)
**		target		remote node name
**		[port]		optional IP port number if no ``/etc/services''
*/

	monitor 2;	/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
chkargs:
	match target UNDEFINED paramerr;
	match server UNDEFINED paramerr;
	match localdmnargs UNDEFINED start;
	mode localdmnargs;
	next start;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"target=<IP name of remote site>\"\n"
		"\t-D \"server=<server name for port, eg: acsnet>\""
		;

start:
	timeout 60;
	match port UNDEFINED noport;
	open "udp" target server port;
	next checkopen;
noport:
	open "udp" target server;
checkopen:
	match RESULT DEVOK openok;
	fail "Could not connect to " target ", reason: " RESULT;

openok:
/* 	trace "connection successful"; */
	daemon "ENdaemon";
	execdaemon;
