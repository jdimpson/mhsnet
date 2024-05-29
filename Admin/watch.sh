#!/bin/sh
tmp1=/tmp/xx.$$.1
tmp2=/tmp/xx.$$.2
who >$tmp1
while sleep 10
do
	who >$tmp2
	if cmp -s $tmp1 $tmp2
	then
		:
	else
		echo -n '' >/dev/tty
		diff $tmp1 $tmp2 >/dev/tty
		mv $tmp2 $tmp1
	fi
done
