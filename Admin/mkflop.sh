#!/bin/sh
#	Make distribution via floppies.
#
#	dd large file onto multiple floppies.

Usage="[-b<bsize>] [-d<device>] [-format] -k<blocks> file"

device=/dev/rfh0a
eject="eject -f $device"	# `eject' command if available

bsize=10240	# 10k is natural size for `tar'
#blocks=144	# number of `bsize' byte blocks on floppy

format=false

for i
do
	case $i in
	-b*)	bsize=`expr $i : '-b\(.*\)'` ;	shift ;;
	-d*)	device=`expr $i : '-d\(.*\)'` ;	shift ;;
	-k*)	blocks=`expr $i : '-k\(.*\)'` ;	shift ;;
	-f*)	format=true ;			shift ;;
	*)					break ;;
	esac
done

case $# in
0)	echo "Usage: $0 $Usage"; exit 1;;
esac
file=$1

case "$blocks" in
''|0)	echo "calculate number of blocks of size $bsize that can be fitted on floppy"
	echo "try: dd if=/unix bs=$bsize of=$device 2>&1|sed -n '2s/+.*//p'"
	echo "Usage: $0 $Usage"
	exit 1;;
esac

size=`ls -l $file | awk '{print $5}'`

iseek=0
next=1
count=`expr \( \( $size + $bsize - 1 \) / $bsize + $blocks - 1 \) / $blocks`
echo Need $count floppies

while [ $size -gt 0 ]
do
	echo "load floppy $next, and hit RETURN\c"
	read ans
	case $format in
	true)	echo "format $device"
		format $device
		;;
	esac
	b=`expr \( $size + $bsize - 1 \) / $bsize`
	[ $b -gt $blocks ] || blocks=$b
	echo "dd if=$file bs=$bsize iseek=$iseek count=$blocks of=$device"
	dd if=$file bs=$bsize iseek=$iseek count=$blocks of=$device
	$eject
	next=`expr $next + 1`
	iseek=`expr $iseek + $blocks`
	size=`expr $size - $blocks \* $bsize`
done
