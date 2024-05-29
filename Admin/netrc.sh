#!/bin/sh
#
#	@(#)netrc.sh	1.3 93/12/09
#
#	Fetch results of commands via "fetcher" server at destination.

usage='"Usage: `basename $0` [-D server] command [arg] ..."'

flags=
dest=

while [ $# -gt 0 ]
do
	case $1 in
	-D)	dest=$2; shift;				shift;;
	-D*)	dest=`expr $1 : '-D\(.*\)'`;		shift;;
	-\?)	eval echo "$usage";			exit 1;;
	-*)	flags="$flags`expr $1 : '-\(.*\)'`";	shift;;
	*)						break;;
	esac
done

case "$1" in
'')	eval echo "$usage";	exit 1	;;
*)	command=$1;	shift		;;
esac

case "$flags" in
'')					;;
*)	flags="-E$flags"		;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

case "$dest" in
'')	dest=`_bin/netaddr`		;;
esac

privs=`_bin/netprivs`

logname=`expr "$privs" : '\(.*\):.*'`

echo "$logname" "$command" "$@" | _bin/netmsg -Afetcher $flags -D$dest
