#!/bin/sh
#
#	@(#)http.sh	1.8 04/01/22
#
#	Fetch URLs via http server at destination.

#	Change "HTTPSERVER" to show your preferred server address,
#	or add a definition for HTTPSERVER to "/usr/lib/MHSnetparams".

SPOOLDIR=/var/spool/MHSnet
eval `egrep 'HTTPSERVER|SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

case "$HTTPSERVER" in
'')
	HTTPSERVER=`_bin/netaddr`
	;;
esac

usage='name=`basename $0`

echo "Usage:	$name [-D server] <URL> ...
  URLs are fetched by \"server\" [default: $HTTPSERVER]
  and returned via \"netfile\". Include authentication
  and port if required in a URL of the form:
	http://username:password@host:port/path/to/file"

exit 1'

flags=

skip=
for i
do
	case "$skip$i" in
	SKIP*)	skip=;					shift;;
	-D)	HTTPSERVER="$2"; skip=SKIP;		shift;;
	-D*)	HTTPSERVER="`expr $i : '-D\(.*\)'`";	shift;;
	-?)	eval "$usage"				     ;;
	*)						break;;
	esac
done

case $# in
0)	eval "$usage"
	;;
esac

URLs="$@"

case "$HTTPSERVER" in
'')	eval "$usage"
	;;
esac

privs=`_bin/netprivs`

logname=`expr "$privs" : '\(.*\):.*'`

echo $logname $internet $URLs | _bin/netmsg -Ahttpserver $flags -D$HTTPSERVER
