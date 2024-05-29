#!/bin/sh

echo "
Show kernel op.sys/cpu: (add result to checklist and \`installmhs')"
echo "uname -a: " `uname -a`
echo "uname -mr: " `uname -mr`
echo "uname -mrs: " `uname -mrs`

echo "
Show suitable uid/gids:"
egrep '^(none|staff|other|news|fileserver|man|daemon|network|bin|uucp)' /etc/passwd /etc/group

echo "
Show builtin defines in gnu C compiler:"
gcc -E -dM - </dev/null | sort


echo "
Searching for libc.a:"
if [ -s /lib64/libc.so.6 ]
then
	libc=/lib64/libc.so.6
elif [ -s /lib/libc.a ]
then
	libc=/lib/libc.a
elif [ -s /usr/lib/libc.a ]
then
	libc=/usr/lib/libc.a
elif [ -s /usr/ccs/lib/libc.a ]
then
	libc=/usr/ccs/lib/libc.a
elif [ -s /lib/386/Slibc.a ]
then
	libc=/lib/386/Slibc.a
else
	echo "Whereis libc?"
	libc=UNKNOWN
fi

echo "	found $libc

Searching for include files:"
CONFIG='-DINT_SWITCH -DSLOW_MALLOC -DLONG_LONG'

if [ -s /usr/include/errno.h ]
then
	CONFIG="$CONFIG -DERRNO_H"
fi

if [ -s /usr/include/sys/mkdev.h ]
then
	CONFIG="$CONFIG -DMKDEV_H"
fi

if [ -s /usr/include/stdlib.h ]
then
	CONFIG="$CONFIG -DSTDLIB_H"
fi

if [ -s /usr/include/string.h ]
then
	CONFIG="$CONFIG -DSTRING_H"
fi

if [ -s /usr/include/sys/statvfs.h ]
then
	CONFIG="$CONFIG -DSYS_STATVFS"
elif [ -s /usr/include/sys/statfs.h ]
then
	CONFIG="$CONFIG -DSYS_STATFS"
elif [ -s /usr/include/sys/vfs.h ]
then
	CONFIG="$CONFIG -DSYS_VFS"
fi

if [ -s /usr/include/sys/sysmacros.h ]
then
	CONFIG="$CONFIG -DSYSMACROS_H"
fi

if [ -s /usr/include/unistd.h ]
then
	CONFIG="$CONFIG -DUNISTD_H"
fi

if [ ! -s /usr/include/utime.h ]
then
	if egrep 'struct timeval' /usr/include/sys/time.h >/dev/null 2>&1
	then
		CONFIG="$CONFIG -DUTIMES"
		echo '
check "man utime vs. utimes" against Lib/SMdate.c'
	fi
fi

echo "
Makefile MAIL variables:"
if [ -s /usr/lib/sendmail ]
then
	echo "	BINMAIL='/usr/lib/sendmail' \\"
	echo "	BINMAILARGS='\\\"-f&F@&O\\\"' \\"
	echo "	MAILER='/usr/lib/sendmail' \\"
	echo "	MAILERARGS='\\\"-oem\\\",\\\"-oit\\\",\\\"-f&F@&O\\\"' \\"
	echo "	IGNMAILERSTATUS='2' \\"
elif [ -s /usr/sbin/sendmail ]
then
	echo "	BINMAIL='/usr/sbin/sendmail' \\"
	echo "	BINMAILARGS='\\\"-f&F@&O\\\"' \\"
	echo "	MAILER='/usr/sbin/sendmail' \\"
	echo "	MAILERARGS='\\\"-oem\\\",\\\"-oit\\\",\\\"-f&F@&O\\\"' \\"
	echo "	IGNMAILERSTATUS='2' \\"
else
	echo "	BINMAIL='/bin/mail' \\"
	echo "	BINMAILARGS='' \\"
	echo "	MAILER='/bin/mail' \\"
	echo "	MAILERARGS='' \\"
	echo "	IGNMAILERSTATUS='0' \\"
fi

echo "
Makefile CONFIG variable:"
for i in `nm $libc | egrep '(bcopy|dial|fsync|rename|mlock|random|sighold|sigblock|socket|strerror|vfprintf)'`
do
	case "$i" in
	*bcopy*)	bcopy=1; continue		;;
	*dial*)		VAR=UDIAL			;;
	*bufsync*)	continue			;;
	*fsync*)	VAR=FSYNC_2			;;
	*rename*)	rename=1; continue		;;
	*rmlock*)	VAR=UUCPLOCKLIB			;;
	*mlock*)	mlock=1; continue		;;
	*random*)     VAR='Rand=random -DSRand=srandom'	;;
	*sigblock*)	VAR=SIGBLOCK			;;
	*sighold*)	VAR=SIGHOLD			;;
	*socket*)	socket=1; continue		;;
	*strerror*)	strerror=1; continue		;;
	*vfprintf*)	vprintf=1; continue		;;
	esac

	case "$VAR" in
	SIGHOLD)
		if grep SIGHOLD /usr/include/sys/signal.h >/dev/null
		then
			continue
		fi
		;;
	esac

	case "$CONFIG" in
	*"$VAR"*)			;;
	*)	CONFIG="$CONFIG -D$VAR"	;;
	esac
done

case "$bcopy" in
1)					;;
*)	CONFIG="$CONFIG -DNEED_BCOPY"	;;
esac

case "$strerror" in
1)					;;
*)	CONFIG="$CONFIG -DNO_STRERROR"	;;
esac

CONFIG=`echo $CONFIG | tr -s ' ' '\012' | sort | tr -s '\012' ' '`
echo "	CONFIG='$CONFIG' \\"

echo "
Other Makefile variables:"
case "$rename" in
1)					;;
*)	echo "	RENAME_2='0' \\"	;;
esac

case "$vprintf" in
1)					;;
*)	echo "	VPRINTF='0' \\"		;;
esac

case "$mlock" in
1)	echo "
check if mlock(2) exists,
in which case you must add mlock=Mlock to CONFIG and not define UUCPLOCKLIB"
	if [ -s /usr/include/sys/mkdev.h ]
	then
		echo "also check if cu(1) creates a lockfile with
name = ascii representation of device code: \"major(dev).major(rdev).minor(rdev)\"
(eg: \"/usr/spool/locks/LK.016.000.017\")
in which case you must also define:
	UUCPMLOCKDEV='1' \\
	UUCPLCKPRE='LK.' \\"
	fi
	;;
esac

if [ -d /usr/spool/locks ]
then
	echo "	UUCPLCKDIR='/usr/spool/locks' \\"
	echo "	UUCPSTRPID='1' \\"
elif [ -d /usr/spool/uucp/LCK ]
then
	echo "	UUCPLCKDIR='/usr/spool/uucp/LCK' \\"
else
	echo "standard uucp locking"
fi

case "$socket" in
1)	echo "socket code in $libc"; exit 0	;;
esac

echo "searching for socket code in /lib/lib*.a etc ..."
for l in /lib64/tls/*.a /lib64/*.a /usr/lib64/*.a /lib/lib*.a /usr/lib/lib*.a /usr/ccs/lib/lib*.a /lib/386/Slib*.a /lib/*.so.* /usr/lib/*.so*
do
	case "$l" in
	*\**)	continue;;
	esac
	case `nm $l 2>/dev/null | egrep socket` in
	*socket*)	echo "socket code may be in $l"
			socket=1
			;;
	esac
done
case "$socket" in 1) exit 0 ;; esac
echo "can't find socket library"
exit 1
