#!/bin/sh

loginname=`grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'`
case "$loginname" in '') echo "You need to create your site details first."; sleep 2; exit 1;; esac

rm -f $link.brk
eval `awk -f directout.awk ${link}.lnk`
crontime=`sed -n -e 's/^crontime:	"\([^"]*\)"/\1/p' ${link}.lnk`

prompt="a number between 1 and 5 (or q to quit) must be specified"
changing=true
while $changing
do
  echo "

	Select item to change

	1. password at remote machine [$password]

	2. line speed [$linespeed]

	3. serial line device [$modemdev]

	4. link filter [$filter]
"
  case "$crontime" in
  *restart*) echo "	5. connection [maintained at all times]";;
  *runonce*) echo "	5. connection [made explicitly]";;
  *)         echo "	5. connection [$crontime]";;
  esac
  echo "
"
  case "$flags" in
  ''|-call)	echo "	6. auto dial [not set]";;
  *)		echo "	6. auto dial [selected]";;
  esac
  echo "
"

  case `getans "Item (or q to quit)? " "$prompt"` in
  1)	password=`getdefans "password?" "$password" "optional"`;;
  2)	linespeed=`getdefans "line speed?" "$linespeed"`;;
  3)	modemdev=`getdefans "serial line device?" "$modemdev"`;;
  4)	case "$filter" in
	'')	filter=`getdefans "link filter?" "$filter"`;;
	*)	if answeryes "Do you want to remove the filter? "
		then
			filter=
		else
			filter=`getdefans "link filter?" $filter`
		fi;;
	esac;;
  5)	case "$crontime" in
	*restart*)	echo "currently the connection is maintained at all times";;
	*runonce*)	echo "currently the connection is made explicitly using netcontrol";;
	*)		echo "currently the connection is made at regular times according to the cron spec.:"
			echo "$crontime";;
	esac
	echo
	. getcron.sh;;
  6)	case "$flags" in
	''|-call)	echo "auto-call set";	flags=+call; sleep 2;;
	*)		echo "auto-call unset";	flags=-call; sleep 2;;
	esac;;
  q)	changing=false;;
  *)	echo "$prompt";;
  esac
done

sed \
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
echo "login name:	$loginname"	>>$link.lnk
echo "password:	$password"		>>$link.lnk
echo "line speed:	$linespeed"	>>$link.lnk
echo "modem device:	$modemdev"	>>$link.lnk
echo "type:		directout"	>>$link.lnk
echo "link filter:	$filter"	>>$link.lnk
echo "link flags:	$flags"		>>$link.lnk
