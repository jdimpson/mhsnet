#!/bin/sh
SPOOLDIR=/usr/spool/MHSnet
echo=/bin/echo
if [ -s /usr/lib/MHSnetparams ]
then
	eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
	case `$echo -n hi` in
	-n*)	n=""; c="\c"	;;
	*)	n="-n"; c=""	;;
	esac
	$echo $n "remove /usr/lib/MHSnetparams? "$c
	read ans
	case "$ans" in
	y*)	rm /usr/lib/MHSnetparams;;
	esac
fi
case `pwd` in
*_config)	cd ..		;;
*)		cd $SPOOLDIR	;;
esac
( cd _config; sh clean.sh; )
rm -f \
	*/*BAD \
	*/*.back \
	*/*.lock \
	*/*.log \
	*/*.ol \
	*/*.ool \
	*/*.out \
	*/OLD* \
	*/core \
	*/dead.letter \
	*/lock \
	*/log \
	*/olderlog \
	*/oldlog \
	_call/passwd \
	_lib/initfile \
	_lib/MHSnetparams \
	_state/*file \
	_state/commandsfile \
	_state/promiscuous \
	_state/removed.MSGS \
	_state/routefile \
	_state/statefile \
	_stats/Acc* \
	core \
	dead.letter \
	*/[0-9]*
rm -fr \
	_forward/* \
	_messages/* \
	_route/* \
	_setup \
	_state/NOTES \
	_state/MSGS \
	0=au
rm -fr 0=au
rm -fi _params/*
