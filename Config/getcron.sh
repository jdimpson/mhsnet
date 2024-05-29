#!/bin/sh
case "$norestart" in
true)	echo "
	Calls can be made at regular times,
	or started explicitly using \"netcontrol\".
"
	;;
*)	echo "
	Calls can be made at regular times, started explicitly using
	\"netcontrol\", or the connection can be maintained all the time.
"
	;;
esac

if [ -z "$norestart" ] && answeryes "Is the connection to be maintained at all times? "
then
	crontime=restart
elif answeryes "Is the connection to be started explicitly? "
then
	crontime=runonce
else
	echo "
	To specify when the calls will be made you need to
	enter a cron time specification for the times.
	The format is described in the Unix manual entry cron(8).
"

	if answeryes "Do you need a description of the format? "
	then
		cat cron.desc
	fi

	case "$norestart" in
	true)	echo "
	NB: Austel states that an ACU may only make five (5) calls
	within any 30 minute period to the same number.
"
	esac

	crontime=`getans "Cron time specification?" "A crontime must be specified."`
fi
