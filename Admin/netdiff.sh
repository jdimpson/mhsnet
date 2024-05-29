#!/bin/sh
for i in `netget -l|awk '{print $6}'`
do
	echo ====$i====
	netget -ok $i | diff $i -
done
