#!/bin/sh
#
Usage="[-s] [-t] tar-tv1 [tar-tv2] {generate[compare] tar.tv->bol files}
	-s	show sizes
	-t	sort before compare"
#

awkscript1='
{
	mode = $1
	if ( mode == "tar:" )
		next
	if ( $0 ~ / linked to / )
		file = $(NF-3)
	else
	if ( $0 ~ / == / )
		file = $(NF-2)
	else
		file = $NF
	if ( file ~ /^\.$/ )
		file = "./"
	if ( file !~ /^\.\// )
		file = "./" file
	if ( mode !~ /........../ ) {
		if ( file !~ /\/$/ )
			mode = "-" mode
		else
			mode = "d" mode
	}
	else
	if ( mode ~ /^d/ && file !~ /\/$/ )
		file = file "/"
	printf "%s\t%s\n", mode, file
}'

awkscript2='
{
	mode = $1
	if ( mode == "tar:" )
		next
	if ( $0 ~ / linked to / )
		file = $(NF-3)
	else
	if ( $0 ~ / == / )
		file = $(NF-2)
	else
		file = $NF
	if ( file ~ /^\.$/ )
		file = "./"
	if ( file !~ /^\.\// )
		file = "./" file
	if ( mode !~ /........../ ) {
		if ( file !~ /\/$/ )
			mode = "-" mode
		else
			mode = "d" mode
	}
	else
	if ( mode ~ /^d/ && file !~ /\/$/ )
		file = file "/"
	printf "%s\t%s\t%s\n", mode, $3, file
}'

for i
do
	case $i in
	-s)	sizes=true; shift	;;
	-t)	sort=true; shift	;;
	-*)	echo "Usage: $Usage"; exit 1;;
	esac
done

case $# in
1)	generate=true
	file1=$1
	file2=
	;;
2)	generate=false
	file1=$1
	file2=$2
	;;
*)	echo "Usage: $Usage"; exit 1;;
esac

case $generate in
true)	case "$sizes" in
	true)	awk "$awkscript2" <$file1	;;
	*)	awk "$awkscript1" <$file1	;;
	esac
	;;
*)	case "$sizes" in
	true)	awk "$awkscript2" <$file1 >/tmp/bol1.$$
		awk "$awkscript2" <$file2 >/tmp/bol2.$$
		;;
	*)	awk "$awkscript1" <$file1 >/tmp/bol1.$$
		awk "$awkscript1" <$file2 >/tmp/bol2.$$
		;;
	esac
	case "$sort:$sizes" in
	true:)
		sort +1 -o /tmp/bol1.$$ /tmp/bol1.$$
		sort +1 -o /tmp/bol2.$$ /tmp/bol2.$$
		;;
	true:true)
		sort +2 -o /tmp/bol1.$$ /tmp/bol1.$$
		sort +2 -o /tmp/bol2.$$ /tmp/bol2.$$
		;;
	esac
	diff -b /tmp/bol1.$$ /tmp/bol2.$$
	rm -f /tmp/bol1.$$ /tmp/bol2.$$
	;;
esac
