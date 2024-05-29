#!/bin/sh
#
#	System initialisation program to start MHSnet
#	for SYSTEM V: "/etc/rc2.d/S99MHSnet"
#
# This script executes a corresponding script in /etc/rclocal.d.
# If you want to make a special local version of this
# operation, copy the /etc/rclocal.d file on top of this file and
# make your modifications.  Do not change the file in
# /etc/rclocal.d.
#

REALFILE=/etc/rclocal.d/MHSnet

if [ -f "$REALFILE" ]
then
	exec "$REALFILE" ${1+"$@"}
else
	echo "Warning: $REALFILE does not exist"
	exit 1
fi
