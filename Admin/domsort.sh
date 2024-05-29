#!/bin/sh
#
#	@(#)domsort.sh	1.7 95/01/12
#
#	Order region domains in reverse,
#	and then sort the result, leaving other fields intact.
#

usage='"Usage: `basename $0` [host_list ...]
	or as in \"netmap -l|netsort\""'

case $# in
0)	;;
*)	case $1 in
	-?)	eval echo "$usage"; exit 1	;;
	esac
	;;
esac

awk	'
	BEGIN	{ FS = "	"	}
	/^[^ 	.]+\.[^ 	.]/	{
			i = split($1, a, ".")
			printf("%s", a[i])
			while ( --i > 0 )
				printf(".%s", a[i])
			if ( NF > 1 ) {
				for ( i = 2 ; i <= NF ; i++ )
					printf("\t%s", $i)
			}
			printf("\n")
		}
	' $* | sort -f
