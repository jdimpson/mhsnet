#!/bin/sh
#
#	show link information
#
trap 'menu links.menu; exit 0' 2
echo=/bin/echo
$echo $n "What is the node name of the remote machine? $c"
read link

if [ -f $link.lnk ]
then
	echo
	echo 'Existing link information is:'
	echo
	cat $link.lnk
	echo
else
	echo "There is no link called \"$link\""
	echo
fi
$echo $n "- hit return to continue -$c"
read x
