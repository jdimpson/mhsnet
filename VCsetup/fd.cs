/*
**	Script to setup pre-established ``tty'' circuit pased in as a file-decriptor.
**
**	Invoke via eg:-
**		 VCcall \
**			-D dmnargs=-D512 \ [OR -D localdmnargs=-D512 \ ]
**			-D dmnname=daemon \[optional]
**			-D linespeed=9600 \[optional]
**			-D fd=1 \
**			-S fd.cs \
**			host
**
**	(Should be run from both ends.)
*/
	monitor 2;				/* Turn on I/O tracing */
	match dmnargs UNDEFINED		chkargs;
	set localdmnargs dmnargs;
chkargs:
	match localdmnargs UNDEFINED paramerr;
	match fd UNDEFINED		paramerr;

	open "fd" fd;
	match RESULT DEVOK		openok;
	fail "Could not use file descriptor " fd ", reason: " RESULT;

paramerr:
	fail "missing some of:\n"
		"\t-D \"localdmnargs=<parameters for transport daemon>\"\n"
		"\t-D \"fd=<file descriptor with connection to remote>\""
		;

openok:
	match linespeed UNDEFINED	setraw;
	device "speed" linespeed;
	match RESULT DEVOK		speedok;
	fail "Could not set speed " linespeed ", reason: " RESULT;

speedok:
	sleep 2;
	device "flush";
	match RESULT DEVOK		flushok;
	set count 10;				/* "flush" unimplemented */

flushcount:
	test count			flushok;
	timeout 2;
	expect
		TIMEOUT			flushok,
		".*"			flushcount;

flushok:
	match dmnname UNDEFINED		execdmn;
	match dmnname "VCdaemon"	execdmn;
	daemon dmnname;				/* Don't send the default */
execdmn:
	execdaemon localdmnargs;

setraw:
	device "raw";
	next				speedok;
