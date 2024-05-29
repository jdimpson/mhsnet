#!/bin/sh
#
#	@(#)novis.sh	1.3 92/06/28
#
#	Find sites without visible regions.
#

usage='"Usage: `basename $0` [-c]
(Prints sites without visible regions.)"'

cmds=false

for i
do
	case "$i" in
	-c)	cmds=true; shift	;;
	esac
done

case $# in
0)	;;
*)	eval echo "$usage"; exit 1	;;
esac

myvis=`netmap -rv|sed -n -e '2s/.* //p' -e 2q`

case "$cmds" in
true)	netmap -zlti world |
	sed \
	    -e 's/^9.*'$myvis'/visible	&	'$myvis'/' \
	    -e 's/^9.*\(0=.*\)/visible	&	\1/'
	;;
*)	netmap -zlti world
	;;
esac
