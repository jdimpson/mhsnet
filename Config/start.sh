#!/bin/sh
trap 'exit 0' 2
chown=chown

case `grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'` in
'') echo "You need to create your site details first."; sleep 2; exit 1;;
esac

if answeryes 'Do you want to install your configuration information? '
then
	ERROR=false
	case `find $SPOOLDIR/_state/commandsfile -newer commandsfile -print 2>/dev/null`
	in
	'')	;;
	*)	echo "Warning: the file \"$SPOOLDIR/_state/commandsfile\"
has been modified since the last configuration"
		if answerno 'Do you want to continue? '
		then
			exit 1
		fi
		cp $SPOOLDIR/_state/commandsfile $SPOOLDIR/_state/commands.save
		echo "Changed version saved in \"$SPOOLDIR/_state/commands.save\""
		;;
	esac
	mkcfile.sh || exit 1
	case `find $SPOOLDIR/_lib/initfile -newer initfile -print 2>/dev/null`
	in
	'')	;;
	*)	echo "Warning: the file \"$SPOOLDIR/_lib/initfile\"
has been modified since the last configuration"
		if answerno 'Do you want to continue? '
		then
			exit 1
		fi
		cp $SPOOLDIR/_lib/initfile $SPOOLDIR/_lib/initfile.save
		echo "Changed version saved in \"$SPOOLDIR/_lib/initfile.save\""
		;;
	esac
	mkifile.sh || exit 1

	echo 'Installing configuration files:'
	echo " copying \"commandsfile\" to $SPOOLDIR/_state"
	cp commandsfile $SPOOLDIR/_state || ERROR=true
	$chown $NETUSERNAME $SPOOLDIR/_state/commandsfile || ERROR=true
	chgrp $NETGROUPNAME $SPOOLDIR/_state/commandsfile || ERROR=true
	chmod 660 $SPOOLDIR/_state/commandsfile
	touch commandsfile

	echo " copying \"initfile\" to $SPOOLDIR/_lib"
	cp initfile $SPOOLDIR/_lib || ERROR=true
	$chown $NETUSERNAME $SPOOLDIR/_lib/initfile || ERROR=true
	chgrp $NETGROUPNAME $SPOOLDIR/_lib/initfile || ERROR=true
	chmod 660 $SPOOLDIR/_lib/initfile
	touch initfile

	case "$ERROR" in
	true)
		echo "A serious error has occured. Your system may not be"
		echo "installed correctly. Please consult your support service."
		sleep 2; exit 1
		;;
	esac
fi

if [ -s ../_lib/lock ]
then
	echo "stopping network"
	../_bin/netcontrol shutdown
	echo
	echo "
Restarting network ...
"
else
	echo "
Starting network ...
"
fi
../_lib/start
