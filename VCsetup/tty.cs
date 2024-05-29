/*
**	Script to establish circuit over direct RS-232 connection.
**
**	Invoke via eg:-
**		 VCcall \
**			-D dmnargs=-D512 \ [OR -D localdmnargs=-D512 \ ]
**			-D linespeed=9600 \
**			-D ttydevice=/dev/ttya \
**			-S tty.cs \
**			host
**
**	(Should be run from both ends.)
*/
	monitor 2;			/* Turn on I/O tracing */
	match dmnargs UNDEFINED chkargs;
	set localdmnargs dmnargs;
chkargs:
	match linespeed UNDEFINED paramerr;
	match localdmnargs UNDEFINED paramerr;
	match ttydevice UNDEFINED paramerr;

	open "tty" ttydevice "uucplock";
	match RESULT DEVOK openok;
	fail "Could not open " ttydevice ", reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"linespeed=<bits/second>\"\n"
		"\t-D \"localdmnargs=<parameters for transport daemon>\"\n"
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
