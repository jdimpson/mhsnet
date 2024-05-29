#!/bin/sh
case $0 in
*g|*grp)prog=chgrp; usage="Usage: $0 gid
	[will change group ownership of all files to gid]";;
*)	prog=chown; usage="Usage: $0 uid
	[will change ownership of all files to uid]";;
esac
case $# in
0)	echo $usage
	exit 1
	;;
esac
uid=$1
$prog $uid . *
$prog $uid [A-K]*/*
$prog $uid L*/*
$prog $uid [M-Z]*/*
$prog $uid NNdaemon/*/*
