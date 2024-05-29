#!/bin/sh

# Run this to create tests in `_tests'.

chown=chown
NETUSERNAME=daemon
SHELL=/bin/sh

SPOOLDIR=/usr/spool/MHSnet
eval `egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null`
cd $SPOOLDIR

test -d _tests || mkdir _tests || exit 1
cd _tests

if [ ! -s data.100k ]
then
	tar cf - $SPOOLDIR | compress |
		dd ibs=1k skip=1 count=100 obs=10k >data.100k 2>/dev/null
	dd if=data.100k bs=10k count=1 >data.10k 2>/dev/null
	dd if=data.100k bs=1k count=1 >data.1k 2>/dev/null
	chmod 400 data.*
	$chown $NETUSERNAME data.*
fi

cat >setup <<!
#!$SHELL

# Run this to setup \`.params' for subsequent commands.

SPOOLDIR=/usr/spool/MHSnet
eval \`egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null\`
cd \$SPOOLDIR/_tests

PATH=\$SPOOLDIR/_bin:.:\$PATH

user=\`netprivs|sed 's/:.*//'\`
home=\`netaddr\`
link=\`netmap \$home|tail -1|sed 's/[ 	>-]*//g'\`

cat >.params <<EOF
user=\$user
home=\$home
link=\$link
EOF

cat >.funcs <<EOF
vl(){
	case $# in
	0)	prog='netlink -vc1 \$link | netdis'	;;
	*)	prog='netlink -vc1 \\\$* | netdis'	;;
	esac
	eval \\\$prog
}

vq(){
	netq -aqC1 | netdis
}
EOF

echo "Add \\"-U1 -Z1\\" to \$SPOOLDIR/\`netaddr -l \$link\`/params
	and restart call.

Functions "vl" and "vq" available via ". ./.funcs""
!
chmod 700 setup

cat >clean <<!
#!$SHELL

# Clean out messages bounced from link.

SPOOLDIR=/usr/spool/MHSnet
eval \`egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null\`
cd \$SPOOLDIR/_tests

PATH=\$SPOOLDIR/_bin:.:\$PATH

test -s .params || setup
. .params

netget -sy -delete -U\$user -S\$user
!
chmod 700 clean

cat >bounce1 <<!
#!$SHELL

# Send one message to link and back again.

SPOOLDIR=/usr/spool/MHSnet
eval \`egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null\`
cd \$SPOOLDIR/_tests

PATH=\$SPOOLDIR/_bin:.:\$PATH

case \$# in
0)	file="-X \$SPOOLDIR/_tests/data.10k"	;;
*)	file="\$*"				;;
esac

test -s .params || setup
. .params

netfile -mn \$user@\$link!\$home \$file
!
chmod 700 bounce1

cat >bounce4 <<!
#!$SHELL

# Send 7 messages to link and back again:
# 1*100k at background priority,
# 2*10 at low priority,
# and 4*1k at medium priority.

SPOOLDIR=/usr/spool/MHSnet
eval \`egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null\`
cd \$SPOOLDIR/_tests

PATH=\$SPOOLDIR/_bin:.:\$PATH

clean

for i in 100k 7 10k 4 10k 5 1k 1 1k 2 1k 3 1k 3
do
	case \$i in
	*k)	size=\$i
		continue
		;;
	*)	pri=\$i
		;;
	esac
	bounce1 "-oP\$pri -X \$SPOOLDIR/_tests/data.\$size"
done
!
chmod 700 bounce4

cat >bouncebig <<!
#!$SHELL

# Send large (10Mb) message to link and back again.

SPOOLDIR=/usr/spool/MHSnet
eval \`egrep SPOOLDIR /usr/lib/MHSnetparams 2>/dev/null\`
cd \$SPOOLDIR/_tests

PATH=\$SPOOLDIR/_bin:.:\$PATH

clean

. .params

freeK=\`netparams -f|sed 's/[^0-9]//g'\`

test -s data.10m || {
	test \$freeK -gt 20000 || { echo "Not enough space."; exit 1; }
	for i in 0 1 2 3 4 5 6 7 8 9
	do
		for j in 0 1 2 3 4 5 6 7 8 9
		do
			cat data.100k
		done
	done >data.10m
	freeK=\`expr \$freeK - 10000\`
}

test \$freeK -gt 10000 || { echo "Not enough space."; exit 1; }

netfile -mn \$user@\$link!\$home -X \$SPOOLDIR/_tests/data.10m
!
chmod 700 bouncebig
