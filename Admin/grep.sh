#!/bin/sh

case $# in
1)	;;
*)	echo "Usage: $0 pattern
	[will search for <pattern> in source]"
	exit 1;;
esac

case $0 in
*l)	flag=-l
	;;
*)	flag=-n
	echo "egrep $flag '"$1"' Makefile */Makefile Manuals/*.* */*.h */*.c"
	;;
esac

egrep $flag "$1" Makefile */Makefile NNdaemon/MakefileP Makefiles/[A-Z]*
egrep $flag "$1" Makefiles/run*
egrep $flag "$1" */llib-* Documents/* Manuals/*.[1-8]
egrep $flag "$1" NNdaemon/*/*.[ch]
egrep $flag "$1" */*.sh Config/*mhs
egrep $flag "$1" [A-E]*/*.[ch]
egrep $flag "$1" [F-K]*/*.[ch]
egrep $flag "$1" L*/*.[ch]
egrep $flag "$1" [M-Z]*/*.[ch]

exit 0
