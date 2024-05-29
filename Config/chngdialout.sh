#!/bin/sh

rm -f $link.brk
eval `awk -f dialout.awk ${link}.lnk`
crontime=`sed -n -e 's/^crontime:	"\([^"]*\)"/\1/p' ${link}.lnk`
case "$linespeed" in '') linespeed=$fixspeed;; esac
case "$fixspeed" in '') fixspeed=no;; *) fixspeed=yes;; esac

prompt="a number between 0 and 11 (or q to quit) must be specified"
changing=true
while $changing
do
  echo "\
	Select item to change

	 0. login name at remote machine [$loginname]
	 1. password at remote machine [$password]
	 2. phone number of remote machine [$telno]
	 3. 2nd phone number for remote machine [$telno2]
	 4. line speed [$linespeed]
	 5. serial line device [$modemdev]
	 6. modem type [$modem]"
  case "$flags" in
  ''|-call)	echo "	 7. auto dial [not set]";;
  *)		echo "	 7. auto dial [selected]";;
  esac
		echo "	 8. link filter [$filter]"
  case "$crontime" in
  *restart*) echo "	 9. connection [maintained at all times]";;
  *runonce*) echo "	 9. connection [made explicitly]";;
  *)         echo "	 9. connection [$crontime]";;
  esac
  case "$cooked" in
  -CX)	     echo "	10. circuit XON/XOFF cooking [$cooked]";;
  -c)	     echo "	10. circuit full cooking [$cooked]";;
  *)	     echo "	10. circuit not cooked";;
  esac
  case "$annexhost" in
  '')	     echo "	11. no annex";;
  *)	     echo "	11. annex params: $annexhost/$annexuser/$annexpass";;
  esac

  case `getans "Item (or q to quit)? " "$prompt"` in
  0)	loginname=`getdefans "login name?" "$loginname"`;;
  1)	password=`getdefans "password?" "$password" "optional"`;;
  2)	telno=`getdefans "phone number of the remote machine?" "$telno"`;;
  3)	telno2=`getdefans "2nd phone number for the remote machine?" "$telno2"`;;
  4)	linespeed=`getdefans "line speed?" "$linespeed"`
	fixspeed=`getdefans "line speed fixed?" "$fixspeed"`
	case "$fixspeed" in n*) fixspeed=no;; *) fixspeed=yes;; esac;;
  5)	modemdev=`getdefans "serial line device?" "$modemdev"`;;
  6)	modem=`getdefans "modem type?" "$modem"`;;
  7)	case "$flags" in
	''|-call)	echo "auto-call set";	flags=+call; sleep 2;;
	*)		echo "auto-call unset";	flags=-call; sleep 2;;
	esac;;
  8)	case "$filter" in
	'')	filter=`getans "link filter?"`;;
	*)	if answeryes "Do you want to remove the filter? "
		then
			filter=
		else
			filter=`getdefans "link filter?" "$filter"`
		fi;;
	esac;;
  9)	case "$crontime" in
	*restart*)	echo "currently the connection is maintained at all times";;
	*runonce*)	echo "currently the connection is made explicitly using netcontrol";;
	*)		echo "currently the connection is made at regular times according to the cron spec.:"
			echo "$crontime";;
	esac
	echo
	norestart=true
	. getcron.sh;;
 10)	case "$cooked" in
	''|no)	if answeryes "Does the circuit need full cooking? "
		then
			cooked=-c
		else
			cooked=-CX
		fi
		;;
	*)	if answeryes "Does the circuit need \"cooking\" ? "
		then
			if answeryes "Does the circuit need full cooking? "
			then
				cooked=-c
			else
				cooked=-CX
			fi
		else
			cooked=no
		fi
		;;
	esac;;
 11)	case "$annexhost" in
	'')	annexuser=`getans "What is the user name?"`
		annexpass=`getans "What is the password?"`
		annexhost=`getans "What is the host name?" "A host name must be given"`
		;;
	*)	if answeryes "Is the remote modem attached to an Annex ? "
		then
			annexuser=`getans "What is the user name?"`
			annexpass=`getans "What is the password?"`
			annexhost=`getans "What is the host name?" "A host name must be given"`
		else
			annexhost=
		fi;;
	esac;;
  q)	changing=false;;
  *)	echo "$prompt";;
  esac
done

sed \
	-e "/^annex/d" \
	-e "/^crontime:/d" \
	-e "/^line cookedX:/d" \
	-e "/^line cooked:/d" \
	-e "/^line fixspeed:/d" \
	-e "/^line speed:/d" \
	-e "/^link filter:/d" \
	-e "/^link flags:/d" \
	-e "/^login name:/d" \
	-e "/^modem device:/d" \
	-e "/^modem type:/d" \
	-e "/^password:/d" \
	-e "/^phone number:/d" \
	-e "/^phone number2:/d" \
	-e "/^reliable:/d" \
	-e "/^remote alias:/d" \
	-e "/^type:/d" \
	${link}.lnk >${link}.tmp
mv ${link}.tmp ${link}.lnk

echo "crontime:	\"$crontime\""		>>$link.lnk
echo "modem type:	$modem"		>>$link.lnk
echo "modem device:	$modemdev"	>>$link.lnk
echo "login name:	$loginname"	>>$link.lnk
echo "password:	$password"		>>$link.lnk
echo "phone number:	$telno"		>>$link.lnk
echo "type:	dialout"		>>$link.lnk
echo "link filter:	$filter"	>>$link.lnk
echo "link flags:	$flags"		>>$link.lnk

case "$telno2" in '');; *) echo "phone number2:	$telno2"	>>$link.lnk;; esac

case "$fixspeed" in
y*)	echo "line fixspeed:	$linespeed"	>>$link.lnk;;
*)	echo "line speed:	$linespeed"	>>$link.lnk;;
esac

case "$cooked" in
-CX)		echo "line cookedX:"	>>$link.lnk;;
-c|yes|on)	echo "line cooked:"	>>$link.lnk;;
esac

case "$annexhost" in
'')	;;
*)		echo "annexhost:	$annexhost"	>>$link.lnk
	case "$annexuser" in
	'')	;;
	*)	echo "annexuser:	$annexuser"	>>$link.lnk
		;;
	esac
	case "$annexpass" in
	'')	;;
	*)	echo "annexpass:	$annexpass"	>>$link.lnk
		;;
	esac
	;;
esac
