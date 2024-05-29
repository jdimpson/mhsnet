#!/bin/sh

if [ -s getaddr.tmp ]
then
. getaddr.tmp
fi

case "$COUNTRY" in
'')	ans=`getans "Country code (or top level domain name)?" "A top level domain name must be specified."`;;
*)	ans=`getdefans "Country code (or top level domain name)?" "$COUNTRY"`;;
esac

case "$COUNTRY" in
$ans)	;;
*)	COUNTRY=$ans
	PRMD=; org=; dept=; group=; node=;;
esac

case "$PRMD" in
'')	ans=`getans "Network code?"`;;
*)	ans=`getdefans "Network code?" "$PRMD" "optional"`;;
esac

case "$PRMD" in
$ans)	;;
*)	PRMD=$ans
	org=; dept=; group=; node=;;
esac

case "$org" in
'')	ans=`getans "Organisation code?"`;;
*)	ans=`getdefans "Organisation code?" "$org" "optional"`;;
esac

case "$org" in
$ans)	;;
*)	org=$ans
	dept=; group=; node=;;
esac

case "$org" in
'')	;;
*)
	case "$dept" in
	'')	ans=`getans "Department code?"`;;
	*)	ans=`getdefans "Department code?" "$dept" "optional"`;;
	esac

	case "$dept" in
	$ans)	;;
	*)	dept=$ans
		group=; node=;;
	esac

	case "$dept" in
	'')	;;
	*)
		case "$group" in
		'')	ans=`getans "Sub-dept code?"`;;
		*)	ans=`getdefans "Sub-dept code?" "$group" "optional"`;;
		esac

		case "$group" in
		$ans)	;;
		*)	group=$ans
			node=;;
		esac
		;;
	esac
	;;
esac

case "$node" in
'')	node=`getans "Node name?" "A node name must be specified."`;;
*)	node=`getdefans "Node name?" "$node"`;;
esac

Cdom="$COUNTRY"; Cvar="COUNTRY=$COUNTRY"
case "$PRMD" in
'')	PRMDdom=""; PRMDvar="";;
*)	PRMDdom="$PRMD."; PRMDvar="PRMD=$PRMD.";;
esac
case "$org" in
'')	ORGdom=""; ORGvar="";;
*)	ORGdom="$org."; ORGvar="ORG=$org.";;
esac
case "$dept" in
'')	DEPTdom=""; DEPTvar="";;
*)	DEPTdom="$dept."; DEPTvar="DEPT=$dept.";;
esac
case "$group" in
'')	GROUPdom=""; GROUPvar="";;
*)	GROUPdom="$group."; GROUPvar="GROUP=$group.";;
esac
NODEdom="$node."; NODEvar="NODE=$node."

echo > getaddr.tmp || {
	echo "A serious error has occurred, a necessary file cannot"
	echo "be created. Your system may not be installed correctly."
	echo "Please consult your support service."
	sleep 2; exit 1; }

echo addr=\"$NODEvar$GROUPvar$DEPTvar$ORGvar$PRMDvar$Cvar\" >>getaddr.tmp
echo saddr=\"$NODEdom$GROUPdom$DEPTdom$ORGdom$PRMDdom$Cdom\" >>getaddr.tmp
echo region=\"$ORGvar$PRMDvar$Cvar\" >>getaddr.tmp
echo COUNTRY=\"$COUNTRY\" >>getaddr.tmp
echo PRMD=\"$PRMD\" >>getaddr.tmp
echo org=\"$org\" >>getaddr.tmp
echo dept=\"$dept\" >>getaddr.tmp
echo group=\"$group\" >>getaddr.tmp
echo node=\"$node\" >>getaddr.tmp
