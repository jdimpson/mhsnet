#!/bin/sh

case $# in
5)	;;
*)	echo "Usage: $0 user group chownprog user2 group2"
	exit 1
	;;
esac

user=$1
group=$2
chown=$3
user2=$4
group2=$5

case `ls -ldg . | wc -w` in
*8*)	arg=-ild
	;;
*9*)	arg=-ildg
	;;
*)	echo "Unknown \"ls -l\" format -- please fix $0, and re-run"
	exit 1
	;;
esac

outfile=_config/chmod_files
touch $outfile
chmod 700 $outfile
$chown $user $outfile
chgrp $group $outfile

case "$chown" in
*/*)	chownpath=`dirname $chown`
	chown=`basename $chown`
	;;
*)	chownpath=/bin
	;;
esac

case `uname -mrs` in
Linux*armv6l)	awkflag=	;;
Linux*)	awkflag=--traditional	;;
*)	awkflag=		;;
esac

(ls $arg . `find _[a-e]* -print`; ls $arg `find _[f-z]* -print`) |
awk $awkflag '
	BEGIN	{
		nogroup=0
		mode=0
		width=80
		printf("case $# in\n 2) ;;\n *) echo \"$0 arg error (need: user group)\"; exit 1;;\nesac\n")
		printf("user=$1\ngroup=$2\n")
		printf("PATH='$chownpath':$PATH\n");
	}
	/ [dwrxs-][rwxstT-]*  *[0-9][0-9]* [a-z][a-z]*  *[0-9][0-9]* /	{
		nogroup=1
		printf("ERROR: UNKNOWN GROUP IN LINE \"%s\"\n", $0)
	}
	/ [dwrxs-][rwxstT-]*  *[0-9][0-9]* [0-9][0-9]*  *[a-z][a-z]* /	{
		nogroup=1
		printf("ERROR: UNKNOWN GROUP IN LINE \"%s\"\n", $0)
	}
	/r--r--r--/	{
		mode=444
	}
	/rw-------/	{
		mode=600
	}
	/rw-r-----/	{
		mode=640
	}
	/rw-r--r--/	{
		mode=644
	}
	/rw-rw----/	{
		mode=660
	}
	/rw-rw-r--/	{
		mode=664
	}
	/rws--x---/	{
		mode=4710
	}
	/rws--x--T/	{
		mode=5710
	}
	/rws--x--t/	{
		mode=5711
	}
	/rwx--x--t/	{
		mode=1711
	}
	/rws--x--x/	{
		mode=4711
	}
	/rwsr-xr-t/	{
		mode=5755
	}
	/rwsr-xr-x/	{
		mode=4755
	}
	/rwx------/	{
		mode=700
	}
	/rwx-----T/	{
		mode=1700
	}
	/rwx--x---/	{
		mode=710
	}
	/rwx--x--x/	{
		mode=711
	}
	/rwxr-x---/	{
		mode=750
	}
	/rwxrwx---/	{
		mode=770
	}
	/rwxr-xr-t/	{
		mode=1754
	}
	/rwxr-xr-x/	{
		mode=755
	}
	/rwxrwxr-x/	{
		mode=775
	}
	/./		{
		if ( inode[$1]++ )
			next
		if ( nogroup ) {
			file=$9
			group="'$group'"
		} else {
			file=$10
			group=$5
		}
		user=$4
		if ( mode == 0 )
			printf("ERROR: UNKNOWN MODE \"%s\" FOR FILE \"%s\"\n", $2, file)
		if ( user ~ /^'$user'$/ ) {
			user="$user"
		} else if ( user !~ /^'$user2'$/ && user !~ /^root$/ && user !~ /^bin$/ )
			printf("ERROR: UNKNOWN USER \"%s\" FOR FILE \"%s\"\n", user, file)
		if ( group ~ /^'$group'$/ ) {
			group="$group"
		} else if ( group !~ /^'$group2'$/ && group !~ /^bin$/ )
			printf("ERROR: UNKNOWN GROUP \"%s\" FOR FILE \"%s\"\n", group, file)
		chown[user] += 1
		count = chown[user]-1
		chownfiles[user count] = file
		chgrp[group] += 1
		count = chgrp[group]-1
		chgrpfiles[group count] = file
		chmod[mode] += 1
		count = chmod[mode]-1
		chmodfiles[mode count] = file
		nogroup=0
		mode=0
	}
	END	{
		for ( user in chown ) {
			count = chown[user]
			while ( count > 0 ) {
				printf("%s %s", "'$chown'", user)
				for ( l = length("'$chown'" " " user) ; l < width ; ) {
					if ( --count < 0 )
						break
					printf(" %s", chownfiles[user count])
					l += length(" " chownfiles[user count])
				}
				printf("\n")
			}	
		}
		for ( group in chgrp ) {
			count = chgrp[group]
			while ( count > 0 ) {
				printf("chgrp %s", group)
				for ( l = length("chgrp " group) ; l < width ; ) {
					if ( --count < 0 )
						break
					printf(" %s", chgrpfiles[group count])
					l += length(" " chgrpfiles[group count])
				}
				printf("\n")
			}	
		}
		for ( mode in chmod ) {
			count = chmod[mode]
			while ( count > 0 ) {
				printf("chmod %s", mode)
				for ( l = length("chmod " mode) ; l < width ; ) {
					if ( --count < 0 )
						break
					printf(" %s", chmodfiles[mode count])
					l += length(" " chmodfiles[mode count])
				}
				printf("\n")
			}	
		}
	}
' >$outfile || {
	echo "awk error in $0 -- please fix and re-run"; exit 1
}
chmod 700 $outfile
egrep ERROR $outfile && {
	echo "mode error in $0 -- please fix and re-run"; exit 1
}
exit 0
