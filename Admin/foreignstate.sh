#!/bin/sh
#
#	@(#)foreignstate.sh	1.2 92/08/20
#
#	Broadcast a topology message for a `foreign' MHSnet link.

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=$PATH:$SPOOLDIR/_bin

getstr(){
	while [ $# -gt 1 ]
	do
		default=
		optional=
		case "$1" in
		-o)	optional=true
			shift
			;;
		-d)	default="$2"
			shift; shift
			;;
		esac
	done
	while :
	do
		echo "$1 : \c" 1>&2
		read answer
		case "$answer/$default/$optional" in
		//)	;;
		/*/)	echo "$default"
			return
			;;
		*)	echo "$answer"
			return
			;;
		esac
	done
}

echo "
Site address for your foreign link:
"
N=`getstr "Node                   "`
U=`getstr -o "Sub-Dept. [none]       "`
D=`getstr -o "Department [none]      "`
O=`getstr "Organization           "`
P=`getstr -d "oz" "PRMD [oz]              "`
C=`getstr -d "au" "Country [au]           "`

while :
do
	V=`getstr -d "$P" "Visible region [$P]    "`
	case "$V" in
	$O)	V="3=$O.2=$P.0=$C"; break	;;
	$P)	V="2=$P.0=$C"; break		;;
	$C)	V="0=$C"; break			;;
	"$D")	V="4=$D.3=$O.2=$P.0=$C"; break	;;
	*)	echo "Visible region must be one of: $D/$O/$P/$C"
		;;
	esac
done

while :
do
	R=`getstr -d "$O" "Restrict address [$O]"`
	case "$R" in
	'')	break				;;
	$O)	R="3=$O.2=$P.0=$C"; break	;;
	$P)	R="2=$P.0=$C"; break		;;
	$C)	R="0=$C"; break			;;
	"$D")	R="4=$D.3=$O.2=$P.0=$C"; break	;;
	*)	echo "Restrict address must be one of: $D/$O/$P/$C"
		;;
	esac
done

while :
do
	B=`getstr -d "$P" "Broadcast region [$P]  "`
	case "$B" in
	$O)	B="3=$O.2=$P.0=$C"; break	;;
	$P)	B="2=$P.0=$C"; break		;;
	$C)	B="0=$C"; break			;;
	"$D")	B="4=$D.3=$O.2=$P.0=$C"; break	;;
	*)	echo "Broadcast region must be one of: $D/$O/$P/$C"
		;;
	esac
done

case $B in
$V)	BRD="*.$B"		;;
*)	BRD="*.$B *.$V"		;;
esac

case "$U/$D" in
/)	ADDRESS="9=$N.3=$O.2=$P.0=$C"
	;;
/*)	ADDRESS="9=$N.4=$D.3=$O.2=$P.0=$C"
	;;
*/)	ADDRESS="9=$N.4=$U.3=$O.2=$P.0=$C"
	;;
*)	ADDRESS="9=$N.5=$U.4=$D.3=$O.2=$P.0=$C"
	;;
esac

echo "
Map information for your foreign link:
"
ORG=`getstr "Organization     "`
PER=`getstr "Contact Person   "`
EML=`getstr "Email Address    "`
PHN=`getstr "Phone Number     "`
LIC=000000
KEY=XXXXXXXX

echo "Enter SNAIL address (press ctrl-d to finish)"
while read x
do
	case "$ADR" in
	'')	ADR="$x"	;;
	*)	ADR="$ADR\\
		$x"
		;;
	esac
done

cd /tmp
real_node=`netaddr -ti`

cat >cmnds.$$ <<!
ordertypes	C;A;P;O;D;N
address		$ADDRESS
visible		$ADDRESS
		$V

organization	$ORG
postal_address	$ADR

licence_number	$KEY$LIC
person		$PER
email_address	$EML
telno		$PHN

add		$real_node
link		$real_node,
		$ADDRESS
restrict	$real_node,
		$ADDRESS
		$R
!

> route.$$
> state.$$

while :
do
	echo "======
Check:
======"
	cat cmnds.$$
	echo
	case `getstr -d n "Ok? [n]"` in
	y*)	break
		;;
	*)	vi cmnds.$$
		;;
	esac
done

chmod 664 cmnds.$$ route.$$ state.$$

echo "netstate -Rroute.$$ -Sstate.$$ -Ccmnds.$$ -on | netrequest -in -S$ADDRESS $BRD"
netstate -Rroute.$$ -Sstate.$$ -Ccmnds.$$ -on | netrequest -in -S$ADDRESS $BRD

rm -f cmnds.$$ route.$$ state.$$
