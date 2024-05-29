#!/bin/sh
echo > commandsfile || {
	echo "A serious error has occurred, a necessary file cannot"
	echo "be created. Your system may not be installed correctly."
	echo "Please consult your support service."
	sleep 2; exit 1; }

echo	'#########################################################'	 >commandsfile
echo	'#	DO NOT EDIT THIS PART OF THE FILE		#'	>>commandsfile
echo	'ordertypes	COUNTRY;ADMD;PRMD;ORG;DEPT;GROUP;NODE'		>>commandsfile
echo	'map		C	COUNTRY'				>>commandsfile
echo	'map		A	ADMD'					>>commandsfile
echo	'map		P	PRMD'					>>commandsfile
echo	'map		O	ORG'					>>commandsfile
echo	'map		D	DEPT'					>>commandsfile
echo	'map		G	GROUP'					>>commandsfile
echo	'map		N	NODE'					>>commandsfile
echo	'#	DO NOT EDIT THIS PART OF THE FILE		#'	>>commandsfile
echo	'#########################################################'	>>commandsfile

MAP="-e s/NODE=/N=/ -e s/GROUP=/G=/ -e s/DEPT=/D=/ -e s/ORG=/O=/ -e s/PRMD=/P=/ -e s/ADMD=/A=/ -e s/COUNTRY=/C=/"

addr=`sed -n $MAP -e "s/^address	//p" site.details`
case "$addr" in
'')		echo "Error: no site details"; sleep 2; exit 1;;
esac
sed $MAP -e "s/visible/visible	$addr/" site.details >>commandsfile

links=`ls -d *.lnk 2>/dev/null`
case X"$links" in
X)	echo "Warning: no links configured"; sleep 2; exit 0;;
esac

for i in *.lnk
do
	(grep "^address" site.details; cat $i) | 
	sed $MAP |
	awk -F"	" -f mkcfile.awk >>commandsfile || exit 1
done

links=`ls -d *.brk 2>/dev/null`
case X"$links" in
X)	;;
*)
	for i in *.brk
	do
		(grep "^address" site.details; cat $i) | 
		sed $MAP |
		awk -F"	" -f mkcfile2.awk >>commandsfile || exit 1
	done
	;;
esac

cfiles=`ls -d Commandsfiles/* 2>/dev/null`
case X"$cfiles" in
X)	;;
*)	cat Commandsfiles/* >>commandsfile;;
esac

exit 0
