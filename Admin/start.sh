#!/bin/sh
#
#	@(#)start.sh	1.47 97/12/05
#
#	Start up MHSnet.
#
#	Takes one optional arg:
#		-p	don't run `netpurge'.
#

#	Ignore SIGHUP
#
trap '' 1

nopurge=false
case $# in
0)	;;
1)	case "$1" in -p) nopurge=true;; esac
	;;
esac

PATH=/bin:/usr/bin:/etc:/etc/bin:/usr/ucb:/usr/local:/usr/local/bin
export PATH
SHELL=/bin/sh
export SHELL
chown=chown
echo=echo
NETUSERNAME=daemon
SPOOLDIR=/var/spool/MHSnet
eval `egrep 'SPOOLDIR|NETUSERNAME' /usr/lib/MHSnetparams 2>/dev/null`
#
case `$echo '\c'` in
\\c)	n="-n"
	c=""
	export c n
	SYS=BSD
	;;
*)	n=""
	c="\c"
	export c n
	SYS=V
	;;
esac

case $SYS in
V)	case `uname -mrs` in
	'OSF1 V'[4]'.'?' alpha')
		;;
	*)	case `ulimit` in
		unlimited)
			;;
		*)
			ulimit 4000000
			;;
		esac
		;;
	esac
	;;
*)	case `uname -mrs` in
	'BSD/OS 2.'*|BSD/386*1.[01]*)
		limit maxproc 128
		limit datasize 32000000
		;;
	esac
	;;
esac

cd $SPOOLDIR
PATH=$PATH:_bin
export PATH

if >_state/Q$$
then
	rm -f _state/Q$$
else
	echo "Must be \"root\" or \"$NETUSERNAME\" to execute $0."
	exit 1
fi
#
#	Check network not running if invoked as `netstart'
#
case $0 in
*netstart)
	if netcontrol -sL _lib
	then
		echo "MHSnet already active."
		exit 1
	fi
	echo "MHSnet starting ..."
	purgeargs="-dlmt"
	;;
*)
	if netcontrol -svL _lib
	then
		pid=`netcontrol -L _lib`
		echo "
WARNING: MHSnet may already be active - check process ID $pid
and use \"kill -15 $pid\" if its name is \"netinit\"
"
	fi
	purgeargs="-adlmt"
	;;
esac
#
#	Set and clean up our log file.
#
out=_lib/start.log

if [ -s $out ]
then
	mv $out _lib/start.ol
fi
>$out
chmod 640 $out
$chown $NETUSERNAME $out
echo >>$out 2>&1
echo "     NETWORK STARTED "`date` >>$out 2>&1
#
#	Check spool space:
#
netparams -fw >>$out 2>&1 ||
	{ echo "MHSnet spool file-system space shortage -- check $SPOOLDIR/$out"; exit 1; }
#
#	Clean the statefile in case of crash,
#	(add `w' to `flags' for warning about unreachable regions).
#
flags=-ersxC
echo "==== netstate $flags" >>$out 2>&1
#
#	(The "su" below may be needed to cure some SYSTEM V.2 /bin/mkdir bugs.)
#	/bin/su $NETUSERNAME -c "netstate $flags" >>$out 2>&1 ||
#
netstate $flags >>$out 2>&1 ||
	{
	  test -f _state/statefile.back && cp _state/statefile.back _state/statefile
	  netstate $flags >>$out 2>&1 ||
	  { echo "MHSnet state information error -- check $SPOOLDIR/$out"; exit 1; }
	}
netparams -lw >>$out 2>&1 ||
	{ echo "MHSnet activation key error -- check $SPOOLDIR/$out"; exit 1; }
#
#	Change over to new link names:
#
if [ -d au ]
then
	_lib/convdirs.sh >>$out 2>&1
fi
#
#	Clean out junk (and remove old lock files).
#
case "$nopurge" in
true)
	echo "==== SKIPPING netpurge" >>$out 2>&1
	;;
*)
	echo "==== netpurge $purgeargs" >>$out 2>&1
	netpurge $purgeargs >>$out 2>&1 ||
		{ echo "MHSnet work area cleanup error -- check $SPOOLDIR/$out"; exit 1; }
	;;
esac
#
#	Compact broadcast messages data-base.
#
echo "==== netcheckdb" >>$out 2>&1
netcheckdb >>$out 2>&1 ||
	{ echo "MHSnet message data-base error -- check $SPOOLDIR/$out"; exit 1; }
#
#	Start daemons.
#
echo "==== netinit" >>$out 2>&1
rm -f _route/STOP _lib/*lock	# Not removed by `purge -al'
_lib/netinit _lib/initfile >>$out 2>&1 ||
	{ echo "MHSnet startup error -- check $SPOOLDIR/$out"; exit 1; }
sleep 4
test -d /var/run && { netcontrol -L _lib >/var/run/netinit; chmod 644 /var/run/netinit; }
#
#	Fetch routing data from links.
#
echo "==== netrequest -l" >>$out 2>&1
netrequest -l 2>/dev/null

exit 0
