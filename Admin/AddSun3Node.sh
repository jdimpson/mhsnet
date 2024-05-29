#!/bin/sh

#	Add a ``foreign'' Sun4 node into Sun3 statefile,
#	and generate a fake Sun3 state message from it.

SUN3SPOOLDIR=/usr/spool/ACSnet
SPOOLDIR=/usr/spool/MHSnet

here=`uname -n`
cd ${SUN3SPOOLDIR}/_lib

echo "Node name? \c";	read node
echo "Domains? \c";	read domains
echo "Hierarchy? \c";	read hierarchy
echo "Comment? \c";	read comment

> route.$$
> state.$$

acsstate -rroute.$$ -sstate.$$ -h${node} -RSVWC <<!

comment		${node}		${comment}
domain		${node}		${domains}
hierarchy	${node}		${hierarchy}

add		${here}		foreign,terminal
link		${here},${node}	msg,-con,intermittent
spooler		${here}		${SPOOLDIR}/_lib/Sun3_4
!

echo "OK? \c"; read ans
if [ "${ans}" = y ]
then
    mv routefile routefile.bak; mv route.$$ routefile
    mv statefile statefile.bak; mv state.$$ statefile
    ./request -ENGh${node}
    mv statefile.bak statefile
    mv routefile.bak routefile
fi
