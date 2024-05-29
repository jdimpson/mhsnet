#!/bin/sh

#	Shell script to be used instead of `/usr/lib/sendmail'.

#	This shell script passes all mail to MHSnet via netfile.

#	DANGER! This script must only be used at sites that
#	have alternate delivery mechanisms to sendmail already
#	in place. In particular, the MHSnet parameters BINMAIL
#	and MAILER must *not* be "/usr/lib/sendmail".

#	Interprets "-f<sender>@<source>" <user>@<address> ...

PATH=/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local:/usr/local/bin
export PATH

for arg
do
	case "$arg" in
	-bd)	exit 0	# sendmail daemon
		;;
	-bm)	shift	# sendmail default
		;;
	-bs)	shift	# use standard SMTP
		;;
	-d*)	shift	# debugging (really?)
		;;
	-F*)	shift	# sets `full name'
		;;
	-f)	from=$2
		shift; shift
		;;
	-f*)	from=`expr "$arg" : '-f\(.*\)'`
		shift
		;;
	-h*)	shift	# hopcount
		;;
	-M*)	ID=-I`expr "$arg" : '-M\(.*\)'`
		;;
	-n*)	shift	# no aliasing
		;;
	-o*)	shift	# options
		;;
	-q*)	shift	# queue options
		;;
	-r)	from=$2
		shift; shift
		;;
	-r*)	from=`expr "$arg" : '-r\(.*\)'`
		shift
		;;
	-t)		# take addresses from stdin
		exec netmail -fios
		;;
	-v)	shift	# verbose
		;;
	-*)	echo "unexpected arg: $arg"
		exit 1
		;;
	*)	break	# remaining args are user addresses
		;;
	esac
done

case $# in
0)	echo "usage: `basename $0` [-f<sender>@<source>] address ..."
	exit 1
	;;
esac

# message body on stdin

netfile -Amailer -R"$from" $ID $*
