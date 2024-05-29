/*
**	Script to establish circuit to a SUNIII site over direct RS-232 connection.
**
**	Imports:
**		[daemonprog]	optional daemon name [def: NNdaemon]
**		[dmnargs]	alternative for localdmnargs
**		linespeed	speed for device
**		localdmnargs	parameters for daemon at calling site
**		ttydevice	device attached to RS-232 link
*/

	monitor 2;			/* Turn on I/O tracing */
	match daemonprog UNDEFINED dfltdmn;
chkargs1:
	match dmnargs UNDEFINED chkargs2;
	set localdmnargs dmnargs;
chkargs2:
	match linespeed UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match ttydevice UNDEFINED paramerr;

	daemon daemonprog;
	open "tty" ttydevice "uucplock" "local";
	match RESULT DEVOK openok;
	fail "Could not open " ttydevice ", reason: " RESULT;

dfltdmn:
	set daemonprog "NNdaemon";
	next chkargs1;

paramerr:
	fail "missing some of:\n"
		"\t-D \"daemonprog=NNdaemon\"\n"
		"\t-D \"dmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"ttydevice=<device name for connection to remote>\""
		;

openok:
	device "speed" linespeed;
	match RESULT DEVOK speedok;
	fail "Could not set speed " linespeed ", reason: " RESULT;

speedok:
	sleep 2;
	device "flush";
	match RESULT DEVOK flushok;
	set count 10;			/* "flush" unimplemented */

flushcount:
	test count flushok;
	timeout 2;
	expect
		TIMEOUT flushok,
		".*" flushcount;

flushok:
	forkdaemon localdmnargs;
	return;
