#!/bin/sh

case $# in
2);;
*)	echo "Usage: $0 oldfile newfile"; exit 1;;
esac

case "$PATH" in
*Bin*)	;;
*)	PATH=$S/Bin:$PATH
	export PATH
	;;
esac

touch -c `datenum -t $1` $2

ls -ld $1 $2
