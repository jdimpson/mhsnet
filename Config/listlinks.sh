#!/bin/sh
echo=/bin/echo
links=`ls -d *.lnk 2>/dev/null`
case X"$links" in
X)	echo "- no links configured -";
	sleep 2; exit 1 ;;
esac

for i in  *.lnk
do
	echo $i|sed -e "s/.lnk//"
done | sort
$echo $n "- hit return to continue -$c"
read x
