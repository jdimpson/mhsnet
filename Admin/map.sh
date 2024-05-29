#!/bin/sh
#
#	@(#)map.sh	1.1 89/04/03
#
#	Shell script to "map" all Sun3
#	nodenames into Sun4 routing tables:

netmap -lti|sed 's/9=\([^\.]*\)/map	\1	&/' >mapfile

cat <<!
#	Edit "mapfile" to remove duplicate
#	nodenames (choose your favourite),
#	then run "netstate -crs <mapfile".
!
