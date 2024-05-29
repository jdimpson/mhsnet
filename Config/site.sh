#!/bin/sh
trap 'exit 0' 2
echo=/bin/echo

if [ -f site.details ]
then
	echo
	echo 'Your site information is:'
	echo
	cat site.details
	echo
	if answeryes 'Do you want to edit this information? '
	then
		$EDITOR site.details
		exit 0
	fi
	nodename=`grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'`
	case "$nodename" in
	'') echo "You need to create your site details first."; sleep 2; exit 1;;
	???????????) echo "Warning: nodename is longer than 10 characters";;
	esac
	nodeaddr=`grep '^addr' site.details|sed -e 's/.*	//' -e 's/[A-Z][A-Z]*=//g'`
	nodeaddr=`echo $nodeaddr|tr  '[A-Z]' '[a-z]'`

	case `uname -mrs 2>/dev/null` in
	QNX*|AIX*)	;;
	*)	case "$SYS" in
		BSD)	sysname=`/bin/hostname` >/dev/null 2>&1
			;;
		*)	sysname=`uname -n`
			;;
		esac
		sysname=`echo $sysname|tr  '[A-Z]' '[a-z]'`

		case "$sysname" in
		$nodename|$nodeaddr)
			;;
		*)	echo "The node name in your kernel (System Name)
is not the same as the node name you have given.
"
			if answeryes "Do you want to change your System Name now? "
			then
				case "$SYS" in
				BSD|HP)	echo "/bin/hostname $nodename"
					/bin/hostname $nodename
					if [ -f /etc/netstart ]
					then file="/etc/netstart"
					else file="your boot scripts"; fi
					echo "
You should modify $file to make the change permanent.
"
					;;
				*)	case "SYSTEM" in
					sco_xenix)
						(cd /usr/sys/conf;
						/usr/sys/conf/configure NODE="$nodename"
						echo relinking kernel...
						./link_xenix
						echo installing new kernel...
						./hdinstall
						echo )
						;;
					*)	echo "
Sorry, you will have to consult your administration guide
on how to change the system node name.
"
						;;
					esac
					;;
				esac
			fi
			;;
		esac
		;;
	esac
	if answerno 'Do you want to re-enter this information? '
	then
		exit 0
	fi
fi

echo "
	- Network address of your machine -
"
getaddr.sh
. ./getaddr.tmp

echo "
	- Choose your visible region -

	This is the region that your site is \"visible\" in.
	If this site is connected to other sites in different
	regions, then your visible region should be one that
	all sites are members of, unless other sites are in
	different countries, in which case choose your country.

	One effect of \"visibility\" is that your site will never
	receive from another site a message whose address lies
	outside the visible region. Another effect is that your
	topology information will only be broadcast to other
	sites within the visible region.
"

while true
do
	case "$org" in
	'')	case "$PRMD" in
		'')	vr="$COUNTRY"	 ;;
		*)	vr=`getdefans "Visible region ($PRMD or $COUNTRY)?" "$PRMD"`;;
		esac
		;;
	*)	case "$PRMD" in
		'')	vr=`getdefans "Visible region ($org or $COUNTRY)?" "$org"`	 ;;
		oz)	
			case "$dept" in
			'')	vr=`getdefans "Visible region ($org or $PRMD or $COUNTRY)?" "$PRMD"`	 ;;
			*)
				case "$group" in
				'')	vr=`getdefans "Visible region ($dept or $org or $PRMD or $COUNTRY)?" "$PRMD"`;;
				*)	vr=`getdefans "Visible region ($group or $dept or $org or $PRMD or $COUNTRY)?" "$PRMD"`;;
				esac
				;;
			esac
			;;
		*)
			case "$dept" in
			'')	vr=`getdefans "Visible region ($org or $PRMD or $COUNTRY)?" "$org"`		;;
			*)
				case "$group" in
				'')	vr=`getdefans "Visible region ($dept or $org or $PRMD or $COUNTRY)?" "$org"`;;
				*)	vr=`getdefans "Visible region ($group or $dept or $org or $PRMD or $COUNTRY)?" "$org"`;;
				esac
				;;
			esac
			;;
		esac
		;;
	esac
	case "$vr" in
	$group)		region="G=$group.D=$dept.O=$org.P=$PRMD.C=$COUNTRY"; break	;;
	$dept)		region="D=$dept.O=$org.P=$PRMD.C=$COUNTRY"; break		;;
	$org|'')	region="O=$org.P=$PRMD.C=$COUNTRY"; break			;;
	$PRMD)		region="P=$PRMD.C=$COUNTRY"; break				;;
	$COUNTRY)	region="C=$COUNTRY"; break					;;
	esac
done

case $region in
*.?=.*)	region=`echo $region | sed 's/\.[A-Z]=\././g'`	;;
esac

echo "
	- Site Information -
"
snumber=`getans "Serial number of MHSnet copy?" "The serial number must be specified."`
actkey=`getans "Activation key?" "The activation key must be specified."`
organisation=`getans "Full organisation name?" "An organisation must be specified."`
while true
do
	echo 'Postal address? (terminate with ^D)'
	cat >postaddr
	test -s postaddr && break
	echo 'A postal address must be specified.'
done
telno=`getans "Telephone number?" "A telephone number must be specified."`
contact=`getans "Contact person?" "A contact person must be specified."`
email=`getans "Electronic mail address?" "An e-mail address must be specified."`

echo "
	- Optional Site Information -
"
system=`getans "CPU Type, Operating System?"`
remark=`getans "General remarks?"`
locate=`getans "Location (geographic coordinates,
eg: 33 53 25.0 S / 151 11 18.7 E)?"`

echo	"address	$addr" >site.details || {
	echo "A serious error has occurred, a necessary file cannot"
	echo "be created. Your system may not be installed correctly."
	echo "Please consult your support service."
	sleep 2; exit 1; }

echo	"map	$node	$addr"		>>site.details
echo	'#'				>>site.details
echo	"visible	$region"	>>site.details
echo	'#'				>>site.details
echo    "organisation	$organisation"	>>site.details
$echo $n "postal_address$c"		>>site.details
sed \
	-e '1s/.*/	&\\/' \
	-e '2,$s/.*/		&\\/' \
	-e '$s/\\$//' \
	postaddr			>>site.details
echo 					>>site.details
echo	"serial_number	$actkey$snumber">>site.details
echo	"person		$contact"	>>site.details
echo	"email_address	$email"		>>site.details
echo	"telno		$telno"		>>site.details

case "$system" in '') ;; *)
echo	"system		$system"	>>site.details
;; esac
case "$remark" in '') ;; *)
echo	"remarks		$remark">>site.details
;; esac
case "$locate" in '') ;; *)
echo	"location	$locate"	>>site.details
;; esac

echo "
	Your site information is:
"
cat site.details
echo
if answeryes "Do you want to edit this information? "
then
	$EDITOR site.details
fi

nodename=`grep '^addr' site.details|sed 's/.*NODE=\([^\.]*\).*/\1/'`
case "$nodename" in
'') echo "You need to create your site details first."; sleep 2; exit 1;;
???????????) echo "Warning: nodename is longer than 10 characters";;
esac
nodeaddr=`grep '^addr' site.details|sed -e 's/.*	//' -e 's/[A-Z][A-Z]*=//g'`
nodeaddr=`echo $nodeaddr|tr  '[A-Z]' '[a-z]'`

case "$SYS" in
BSD)	sysname=`/bin/hostname` >/dev/null 2>&1
	;;
*)	sysname=`uname -n`
	;;
esac
sysname=`echo $sysname|tr  '[A-Z]' '[a-z]'`

case "$sysname" in
$nodename|$nodeaddr)
	;;
*)	echo "The node name in your kernel (System Name)
is not the same as the node name you have given.
"
	if answeryes "Do you want to change your System Name now? "
	then
		case "$SYS" in
		BSD|HP)	echo "/bin/hostname $nodename"
			/bin/hostname $nodename
			if [ -f /etc/netstart ]
			then file="/etc/netstart"
			else file="your boot scripts"; fi
			echo "
You should modify $file to make the change permanent.
"
			;;
		*)	case "SYSTEM" in
			sco_xenix)
				(cd /usr/sys/conf;
				/usr/sys/conf/configure NODE="$nodename"
				echo relinking kernel...
				./link_xenix
				echo installing new kernel...
				./hdinstall
				echo )
				;;
			*)	echo "
Sorry, you will have to consult your administration guide
on how to change the system node name.
"
				;;
			esac
			;;
		esac
	fi
	;;
esac
