#!/bin/sh -p
#
#	@(#)pr_sites.sh	1.1 91/04/10
#
#	Format and print output from "nethosts",
#	(possibly sorted by "netsort").
#
#	Will need to be edited for local printer setup.
#

err() {
	echo "Usage: $0 <sites>"; exit 1
}

case $# in
1)	test -r $1 || err;;
*)	err;;
esac

echo "cat $1 | tbl | troff -mm | blw"

( cat <<preamble; sed -e 's/""/\\ /g' $1; echo ".TE" ) | tbl | troff -rN2 -rL11.5i -rW6.375i -rO.3i -rS8 -mm | blw
.PP
.TS H
box expand ;
c2w(1.2i) | c2w(4i) | c2w(2.3i)
l2p-2w(1.2i) | l2p-2w(4i) | l2p-2w(2.3i) .
Address	Organisation	Version
_
.TH
preamble
