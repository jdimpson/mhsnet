#!/bin/sh

if [ -s ../_lib/lock ]
then
	echo "stopping network"
	../_bin/netcontrol shutdown
else
	echo "network inactive"
fi
echo
