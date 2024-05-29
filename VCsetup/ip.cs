/*
**	Call script to connect to a host via an IP network.
**
**	Imports:
**		[dmnname]	alternate transport daemon [default: HTdaemon]
**		[dmnargs]	= localdmnargs = remotedmnargs
**		localdmnargs	parameters for daemon at calling site
**		[passwdstr]	password for login name at remote site
**		[port]		optional IP port number if no `/etc/services'
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
**		server		remote network server name (eg: "mhsnet")
**		service		"udp" or "tcp"
**		[source]	source internet address
**		target		remote internet address
*/

	monitor 2;	/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
	set remotedmnargs dmnargs;
chkargs:
	match target UNDEFINED paramerr;
	match server UNDEFINED paramerr;
	match service UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match remotedmnargs UNDEFINED setrmda;
	next paramsok;

setrmda:
	set remotedmnargs localdmnargs;
	next paramsok;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"target=<IP name of remote site>\"\n"
		"\t-D \"server=<server name for port, eg: mhsnet>\"\n"
		"\t-D \"service=<tcp or udp>\""
		;

paramsok:
	timeout 360;
	match port UNDEFINED noport;
	open service target server port;
	next checkopen;
noport:
	open service target server;
checkopen:
	match RESULT DEVOK openok;
	fail "No connection to " target ", reason: " RESULT;

openok:
	match source UNDEFINED loop;
	device "source" source;
loop:
	set ptimeout 3;
	timeout 60;
	expect
		" error -- " shellerr,
		"Could not exec" shellerr,
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"FAILED PASSWORD" failpasswd,
		"SHELL STARTS 2" namedaemon,
		"SHELL STARTS" params,
		"QUERY PARAMS" params,
		"DAEMON STARTS" startnoshell,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
		EOF eof,
		TIMEOUT failtimeout;

sendhome:
	write "HOMENAME " HOMENAME "\n";
	next loop;

sendpasswd:
	match passwdstr UNDEFINED passwd1;
	write "PASSWORD " passwdstr "\n";
	next loop;

passwd1:
	write "PASSWORD \n";
	next loop;

useHTdaemon:
	set dmnname "HTdaemon";
namedaemon:
	match dmnname UNDEFINED useHTdaemon;
	match dmnname "VCdaemon" params;	/* Don't send the default */
	write "DAEMON " dmnname "\n";
	daemon dmnname;
	next loop;

params:
	test ptimeout failtimeout;
	write "PARAMS " remotedmnargs "\n";
	timeout 5;
	expect
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"QUERY PARAMS" params,
		EOF eof,
		TIMEOUT params;

startdaemon:
	trace "connection successful";
	execdaemon localdmnargs;

alreadyactive:
	fail "remote daemon already active";

eof:
	fail "connection read EOF";

failpasswd:
	fail "password incorrect";

failperm:
	fail "promiscuous connection denied";

failtimeout:
	fail "shell negotiation timeout";

inactive:
	fail "remote network inactive";

shellerr:
	fail "remote " INPUT;

startnoshell:
	trace "connection made without shell negotiation";
	execdaemon localdmnargs;
