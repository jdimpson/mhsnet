#!/bin/sh
#
#	Use "ed" script to disable/enable any getty on "/dev/tty[ab]".
#

usage="Usage: $0 [ttya|ttyb] [on|off]"

device=$1
command=$2

case "$device" in
*cua|*ttya)	getty=ttya		;;
*cub|*ttyb)	getty=ttyb		;;
*)	/bin/echo "$usage"; exit 1	;;
esac

case $command in
on)	edcmd="/$getty/s/^0/1/"		;;
off)	edcmd="/$getty/s/^1/0/"		;;
*)	/bin/echo "$usage"; exit 1	;;
esac

/bin/ed - /etc/ttys <<!
$edcmd
w
q
!

/bin/kill -1 1
