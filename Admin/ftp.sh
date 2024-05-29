#!/bin/sh
#
#	@(#)ftp.sh	1.8 95/04/26
#
#	Fetch files via FTP server at destination.

usage='name=`basename $0`

echo "Usage:	$name [-[a][l][r][v]] [-D server] [-P username@password] \\
		internet_addr file ...
OR:	$name [-opts] internet_addr:file
OR:	$name [-opts] ftp://internet_addr/file"

exit 1'

#	Change "dest" to show your preferred server address.
#	The default is AARNet's "archie" server,
#	which may not be what you want.

dest=plaza.aarnet.oz.au
flags=
user=

skip=
for i
do
	case "$skip$i" in
	SKIP*)	skip=;					shift;;
	-D)	dest="$2"; skip=SKIP;			shift;;
	-D*)	dest="`expr $i : '-D\(.*\)'`";		shift;;
	-P)	user="$2"; skip=SKIP;			shift;;
	-P*)	user="`expr $i : '-P\(.*\)'`";		shift;;
	-*)	flags="$flags`expr $i : '-\(.*\)'`";	shift;;
	*)						break;;
	esac
done

case "$1" in
'')	eval "$usage"
	;;
ftp://*)
	internet=`expr "$1" : 'ftp://\([^/]*\)'`
	files=`expr "$1" : 'ftp://[^/]*/\(.*\)'`
	case "$flags:$files" in *l*:) files=.;; esac
	;;
*:*)
	internet=`expr "$1" : '\([^:]*\)'`
	files=`expr "$1" : '[^:]*:\(.*\)'`
	case "$flags:$files" in *l*:) files=.;; esac
	;;
*)	internet=$1
	;;
esac

shift

case "$*:$files" in
:)	eval "$usage"
	;;
*:)	files="$@"
	;;
:*)	;;
*)	eval "$usage"
	;;
esac

case "$flags" in
'')	;;
*)	case "`echo $flags | egrep '[^alrv]'`" in
	'')	;;
	*)	eval "$usage"
		;;
	esac
	flags="-E$flags"
	;;
esac

case "$user" in
'')	;;
*@*)	uname="`expr "$user" : '\([^@]*\)'`"
	upass="`expr "$user" : '.*@\(.*\)'`"
	;;
*)	eval "$usage"
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

case "$dest" in
'')	dest=`_bin/netaddr`		;;
esac

privs=`_bin/netprivs`

logname=`expr "$privs" : '\(.*\):.*'`

case "$user" in
'')	echo $logname $internet $files
	;;
*)	echo $logname -3 $internet $uname $upass $files
	;;
esac | _bin/netmsg -Aftpserver $flags -D$dest
