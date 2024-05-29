#!/bin/sh
cp initfile.skel initfile || {
	echo "A serious error has occurred, a necessary file cannot"
	echo "be created. Your system may not be installed correctly."
	echo "Please consult your support service."
	sleep 2; exit 1; }

links=`ls -d *.lnk 2>/dev/null`
case X"$links" in
X)	echo "Warning: no links configured"; sleep 2; exit 0;;
X.lnk)	echo "Error: no linkname generated"; sleep 2; exit 1;;
esac

for i in *.lnk
do
	awk -F"	" -f mkifile.awk $i >>initfile
done

ifiles=`ls -d Initfiles/* 2>/dev/null`
case X"$ifiles" in
X)	;;
*)	cat Initfiles/* >>initfile;;
esac

exit 0
