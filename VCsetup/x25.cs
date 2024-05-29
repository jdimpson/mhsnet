/*
**	Call script to connect to a host via an X.25 network.
**
**	Imports:
**		address		remote node's X.25 address
**		controller	local X.25 controller number (default 0)
**		[dmnargs]	= localdmnargs = remotedmnargs
**		home		local node name
**		line		local X.25 line number (default 0)
**		localdmnargs	parameters for daemon at calling site
**		[passwdstr]	password for logina name at remote site
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
*/

	monitor 2;		/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
	set remotedmnargs dmnargs;
chkargs:
	match localdmnargs UNDEFINED paramerr;
	match address UNDEFINED paramerr;
	match home UNDEFINED sethome;
	next bparamsok;

sethome:
	set home HOMENAME;
	next bparamsok;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"address=<X.25 address>\""
		;

bparamsok:
	match controller UNDEFINED setctrlr;
	match controller "[01]" cparamsok;
setctrlr:
	set controller "0";
cparamsok:
	match line UNDEFINED setline;
	match line "[0-9]+" paramsok;
setline:
	set line "0";
paramsok:
	mode localdmnargs;

	open "x25" address controller line home;
	match RESULT DEVOK openok;
	fail "Could not connect to " address ", reason: " RESULT;

openok:
	set ptimeout 3;
	timeout 20;
	expect
		" error -- " shellerr,
		"QUERY HOMENAME" sendhome,
		"CONNECTION DISALLOWED" failperm,
		"QUERY PASSWORD" sendpasswd,
		"FAILED PASSWORD" failpasswd,
		"SHELL STARTS" params,
		"DAEMON STARTS" startnoshell,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		"NETWORK INACTIVE" inactive,
		EOF eof,
		TIMEOUT starttimeout;

sendhome:
	write "HOMENAME " HOMENAME "\n";
	next openok;

sendpasswd:
	match passwdstr UNDEFINED setpw;
passwd1:
	write "PASSWORD " passwdstr "\n";
	next openok;
setpw:
	set passwdstr "";
	next passwd1;

paramsout:
	test ptimeout starttimeout;
params:
	match remotedmnargs UNDEFINED setparams;
params2:
	write "PARAMS " remotedmnargs "\n";
	timeout 5;
	expect
		"DAEMON STARTS" startdaemon,
		"DAEMON ALREADY ACTIVE" alreadyactive,
		EOF eof,
		TIMEOUT paramsout;

setparams:
	set remotedmnargs localdmnargs;		/* "remotedmnargs" defaults to "localdmnargs" */
	next params2;

startdaemon:
	trace "connection successful";
	execdaemon;

alreadyactive:
	fail "remote daemon already active";

eof:
	fail "connection read EOF";

failpasswd:
	fail "password incorrect";

failperm:
	fail "promiscuous connection denied";

inactive:
	fail "remote network inactive";

shellerr:
	fail "remote " INPUT;

startnoshell:
	trace "connection made without shell negotiation";
	execdaemon;

starttimeout:
	trace "connection made after shell negotiation timeout";
	execdaemon;
