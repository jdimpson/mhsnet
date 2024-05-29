#!/bin/sh

visible=`grep '^visible' site.details|sed 's/^[^	]*	\(.*\)/\1/'`

sed -e "/restrict:/d" ${link}.lnk >${link}.tmp
mv ${link}.tmp ${link}.lnk

case "$visible" in
'')	echo "You need to create your site details first."
	sleep 2; exit 1
	;;
*ORG*)
	exit 0
	;;
esac

echo "
	You may restrict messages arriving from the remote site
	to just those addressed to within your own organisation.
	Otherwise this site may be used to route messages from
	the remote site that are addressed to other sites.
"

if answerno "Do you wish to restrict through-routing?"
then
	exit 0
fi

restrict=`grep '^address' site.details|sed 's/.*\(ORG=.*\)/\1/'`

echo	"restrict:	$restrict"	>>${link}.lnk
