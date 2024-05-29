/*
**	Call script to connect to a host via an XTI interface (presumably X.25).
**
**	Must supply: circuit, local name and transport address.
**	local & remote entries are strings that are looked up in local CMX DB.
**
**	Imports:
**		local		Local node - details in CMX database
**		remote		Remote node - details in CMX database
**		address		remote node's XTI address set
**		circuit		local transport circuit 
**		[service]	service designation (usually t_osi_cots)
**		[dmnargs]	= localdmnargs = remotedmnargs
**		home		local node name
**		localdmnargs	parameters for daemon at calling site
**		[passwdstr]	password for logina name at remote site
**		[remotedmnargs]	parameters for daemon at remote site [default: localdmnargs]
*/

	monitor 3;		/* Turn on I/O tracing */

	/*
	**	Check imported strings.
	*/

	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;		/* "dmnargs" sets both local and remote the same */
	set remotedmnargs dmnargs;
chkargs:
	match localdmnargs UNDEFINED paramerr;
	match local UNDEFINED paramerr;
	match remote UNDEFINED paramerr;
	match address UNDEFINED paramerr;
	match circuit UNDEFINED paramerr;
	match home UNDEFINED sethome;
	next paramsok;

sethome:
	set home HOMENAME;
	next paramsok;

paramerr:
	fail "missing some of:\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"local=<local XTI address>\"\n"
		"\t-D \"remote=<remote XTI address>\"\n"
		"\t-D \"circuit=<XTI circuit designation>\"\n"
		"\t-D \"address=<XTI address>\""
		;

noservice:
	open "xti" local remote address circuit "t_osi_cots" home;
	next opendone;

paramsok:
	mode localdmnargs;

	match service UNDEFINED noservice;

	open "xti" local remote address circuit service home;
opendone:
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
	execdaemon localdmnargs;

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
	execdaemon localdmnargs;

starttimeout:
	trace "connection made after shell negotiation timeout";
	trace "params" localdmnargs;
	execdaemon localdmnargs;
