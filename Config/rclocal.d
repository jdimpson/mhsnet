#!/bin/sh
#
#	System initialisation program to start MHSnet
#	for SYSTEM V: "/etc/rclocal.d/MHSnet"
#
SPOOLDIR=/usr/spool/MHSnet

case "$1" in

start)
#	SCOUNIX needs:
#	/bin/su root -c "${SPOOLDIR}/_lib/start"&

	if [ -d ${SPOOLDIR}/_lib -a -f ${SPOOLDIR}/_lib/start ]
	then
		echo Starting MHSnet...
		nohup ${SPOOLDIR}/_lib/start&
	else
		echo "Can't access ${SPOOLDIR}/_lib/start"
	fi
	;;

stop)
#	SCOUNIX needs:
#	/bin/su root -c "${SPOOLDIR}/_bin/netcontrol shutdown"

	test -d ${SPOOLDIR}/_bin -a -f ${SPOOLDIR}/_bin/netcontrol &&
		${SPOOLDIR}/_bin/netcontrol shutdown
	;;

esac
