#!/bin/sh

#	Shell commands to convert all region directory names to typed names.
#
#	This won't work if any of the queues are non-empty,
#	  in which case the messages are just re-routed.

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR|NETUSERNAME' /usr/lib/MHSnetparams 2>/dev/null`
#
cd $SPOOLDIR

netpath=_bin/netpath

if [ -d 0=au ]
then
	exit 0
fi

if >_state/Q$$
then
	rm -f _state/Q$$
else
	echo "Must be \"root\" or \"$NETUSERNAME\" to execute $0."
	exit 1
fi

echo "Changing link names:"

norename=

for i in `find au -name cmds -print`
do
	case `echo $i/*` in
	"$i/*")	;;
	*)	mv $i/* _route	# Reroute messages
		norename=true	# Can't rename dirs containing data files
		;;
	esac
done

case "$norename" in
true)	exit 0;;
esac

for i in `find au -name cmds -print`
do
	path=`expr $i : '\(.*\)/cmds'`
	newpath=`$netpath -p $path`
	newpath=`$netpath -a $newpath`
	opath=
	for j in `echo $newpath|sed 's;/; ;g'`
	do
		oj=`expr $j : '[0-9]=\(.*\)'`
		case "$opath" in
		'')	opath=$oj; npath=$j;;
		*)	opath=$opath/$oj; npath=$npath/$j;;
		esac
		if [ -d $opath ]
		then
			echo mv $opath $npath
			mv $opath $npath
		fi
		opath=$npath
	done
done
