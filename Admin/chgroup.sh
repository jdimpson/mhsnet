#!/bin/sh
case $# in
0)	echo "Usage: $0 gid"
	echo "       [will change group ownership of all files to gid]"
	exit 1;;
*)	;;
esac
gid=$1
chgrp $gid Makefile */Makefile Makefiles/[A-Z]* Makefiles/run* */llib-* Documents/* Manuals/*
chgrp $gid [A-I]*/*.[ch] 
chgrp $gid L*/*.[ch] 
chgrp $gid [M-Z]*/*.[ch] 
chgrp $gid NNdaemon/*/*.[ch]
