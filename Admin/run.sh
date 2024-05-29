#!/bin/sh
runfile=Makefiles/run.your-machine

case $1 in
	-n*)
		shift
		;;
	-y*)	
		mv -f made omade 2>/dev/null
		> made
		shift
		;;
	*)
		case `/bin/echo -n hi` in
		-n*)	n=""; c="\c"	;;
		*)	n="-n"; c=""	;;
		esac
		/bin/echo $n 'Truncate made? (y or n) '$c
		read reply
		case "$reply" in
		[Yy]|[Yy][Ee][Ss])
			mv -f made omade 2>/dev/null
			> made
			;;
		esac
		;;
esac
trap '' 1 2
exec sh $runfile $* >/dev/null 2>&1&
