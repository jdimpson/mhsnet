#!/bin/sh
#
#	@(#)hosts.sh	1.4 92/06/28
#
#	Print out all hosts, one per line.
#	Each line has 3 fields: address<tab>organisation<tab>version
#	Get a domain sort by using "netsort".
#

case $# in
0)	;;
*)	echo "Prints out site names with organisation field.
Very CPU intensive!"
	exit 1;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
PATH=$PATH:$SPOOLDIR/_bin

cd ${SPOOLDIR}/_state
org="
	Organisation:	STATEFILE ERROR"
ver="	Version:	"

for i in `../_bin/netmap -l`
do
	file=`../_bin/netpath -as ${i} MSGS`
	if [ ! -f ${file} ]
	then
		echo ${i}
	else
		../_bin/netmap -wvl ${i} 2>/dev/null || echo "${i}${org}"
		echo "${ver}"`egrep '^(#1\.|1\.[0-9] )' ${file}`
	fi
done |
awk '
	BEGIN					{
			node = ""
			comment = "\"\""
			version = "\"\""
		}
	/^[^	 ]/				{
			if ( node != "" ) {
				printf "%s", node
#				len = length(node)
#				while ( len < 35 ) { printf " "; len++ }
				printf "\t%s\t%s\n", substr(comment, 1, 80), version
			}
			version = "\"\""
			comment = "\"\""
			node = $1
			next
		}
	/^	+Organi[sz]ation:	+/	{
			comment = ""
			for ( i = 2 ; i <= NF ; i++ )
				comment = comment $i " "
			next
		}
	/^	+Phone number:	+/	{
			for ( i = 3 ; i <= NF ; i++ )
				comment = comment $i " "
			next
		}
	/^	+System type:	+/	{
			for ( i = 3 ; i <= NF ; i++ )
				comment = comment $i " "
			next
		}
	/^	+Version:	+/		{
			version = ""
			for ( i = 2 ; i <= NF ; i++ )
				version = version $i " "
			next
		}
	END					{
			if ( node != "" ) {
				printf "%s", node
#				len = length(node)
#				while ( len < 35 ) { printf " "; len++ }
				printf "\t%s\t%s\n", comment, version
			}
		}'
