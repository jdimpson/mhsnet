:
stty echoe echok erase '' kill '' intr ''

umask 022

SHELL=/bin/sh
export SHELL
NAME='Network Manager'
export NAME
EXINIT='set ai terse'
export EXINIT
MORE='-c -s'
export MORE
PAGER=more
export PAGER

EDITOR=/usr/bin/vi
echo=echo
chownprog=/bin/chown
chgrpprog=/bin/chgrp

# USE: `uname -rsv` for AIX as `uname -m` gives model number, rather than architecture.

mach=`uname -mrs`
case "$mach" in
*AIX*)
	echo='/bin/echo'
	chownprog=/usr/bin/chown
	chgrpprog=/usr/bin/chgrp
	;;
BSD/OS' '[23].*|NetBSD*)
	MORE='-cs' export MORE
	echo='/bin/echo'
	chownprog=/usr/sbin/chown
	chgrpprog=/usr/bin/chgrp
	;;
BSD/386*1.[01]*)
	echo='/bin/echo'
	chownprog=/usr/sbin/chown
	chgrpprog=/usr/bin/chgrp
	;;
IRIX*)
	echo=/sbin/echo
	chownprog=/sbin/chown
	chgrpprog=/sbin/chgrp
	;;
'Linux '[12]*)
	echo='echo -e'
	;;
'OSF1 1.'?' alpha')
	echo=/bin/echo
	chownprog=/usr/ucb/chown
	chgrpprog=/usr/ucb/chgrp
	;;
'OSF1 V'[123]'.'?' alpha')
	echo=/bin/echo
	chownprog=/usr/ucb/chown
	chgrpprog=/usr/ucb/chgrp
	;;
'OSF1 V'[45]'.'?' alpha')
	echo=/bin/echo
	;;
QNX*)
	echo='/bin/echo'
	EDITOR=/bin/vi
	;;
'ULTRIX 4.'?' RISC')
	EDITOR=/usr/ucb/vi
	chownprog=/etc/chown
	;;
*4_*mips)
	echo=/bin/echo
	chownprog=/usr/bsd43/bin/chown
	chgrpprog=/usr/bsd43/bin/chgrp
	;;
*5_*mips)
	echo=/bin/echo
	chownprog=/usr/bsd43/bin/chown
	chgrpprog=/usr/bsd43/bin/chgrp
	;;
'SunOS 4'*' sun3')
	echo=/bin/echo
	chownprog=/etc/chown
	EDITOR=/usr/ucb/vi
	;;
'SunOS 4'*' sun4'*)
	echo=/bin/echo
	chownprog=/etc/chown
	EDITOR=/usr/ucb/vi
	;;
'SunOS 5.1 sun4'*)
	echo=/bin/echo
	chownprog=/usr/ucb/chown
	chgrpprog=/usr/ucb/chgrp
	;;
'SunOS 5'*' sun4'*)
	echo=/bin/echo
	chownprog=/usr/ucb/chown
	;;
UNIX*V' 4.'?' R4000')
	echo=/bin/echo
	chownprog=/usr/ucb/chown
	chgrpprog=/usr/bin/chgrp
	;;
ReliantUNIX-N' '5.4*)
	echo=/bin/echo
	;;
*)	echo "what is \"$mach\"?"
esac
export echo chownprog chgrpprog EDITOR

case `$echo -n hi` in
-n*)	n=""
	export n; c="\c"
	export c; _UNIX=V
	;;
*)	case `$echo 'hi\c'` in
	hi)	n=""
		export n; c="\c"
		export c; _UNIX=V
		;;
	*)	n="-n"
		export n; c=""
		export c; _UNIX=BSD
		;;
	esac
	;;
esac

case $_UNIX in
V)	case "$mach" in
	'OSF1 V'[4]'.'?' alpha')
		;;
	*)	ulimit 1000000
		;;
	esac
	;;
esac

case "$_UNIX" in
V)	_HOST=`uname -n`	;;
BSD)	_HOST=`hostname`	;;
esac

case "$_HOST" in
*.*)	_NODE=`expr $_HOST : '\([^.]*\)'`	;;
*)	_NODE="$_HOST"				;;
esac

case "$TERM" in
*xterm)	;;
*)	case "$TERM" in '') TERM=ansi;; esac
	DEFTERM=$TERM
	$echo $n "terminal type (default: $DEFTERM)? "$c
	read TERM
	case "$TERM" in
	''|y|yes)	TERM=$DEFTERM;;
	esac
	unset DEFTERM
	;;
esac
export TERM

case "$TERM$DISPLAY" in
*xterm|*xterm:0|*xterm:0.0)
	if [ -s .Xdisplay ]
	then
		disp=`cat .Xdisplay`
	else
		disp=`uname -n`:0.0
	fi
	$echo $n "DISPLAY= [def: $disp]? "$c
	read DISPLAY
	case "$DISPLAY" in
	'')	DISPLAY=$disp		;;
	*:*)	echo $DISPLAY >.Xdisplay;;
	*)	DISPLAY=$DISPLAY:0.0
		echo $DISPLAY >.Xdisplay;;
	esac
	export DISPLAY
	;;
esac

SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR|NETUSERNAME' /usr/lib/MHSnetparams 2>/dev/null`

case "$EDITOR" in
*vi)	;;
*)	editor=`expr "\`type vi\`" : '[^/]*\([^.)]*\)'`
	case "$editor" in
	/*)	;;
	*)	editor=`which vi`
		;;
	esac
	case "$editor" in
	/*)	EDITOR=$editor
		export EDITOR
		;;
	esac
	;;
esac

I=/usr/include
M=$SPOOLDIR
B=$M/_bin

case "$S" in
'')	test -d Admin && S=`/bin/pwd`
	;;
esac

MC=$M/_config
ML=$M/_lib
MR=$M/_route
MS=$M/_state
MV=$M/_call
export B I M MC ML MR MS MV S

case "$PATH" in
*$M*)	;;
*)	PATH=$PATH:$M/_bin
	export PATH
	;;
esac

case "$PATH" in
*/sbin*)
	;;
*)	test -d /usr/sbin && PATH=/usr/sbin:$PATH
	test -d /sbin && PATH=/sbin:$PATH
	export PATH
	;;
esac

case "$PATH" in
*/usr/etc*)
	;;
*)	test -d /usr/etc && PATH=/usr/etc:$PATH
	;;
esac

case "$PATH" in
*/usr/local/bin*)
	;;
*)	test -d /usr/local/bin && PATH=$PATH:/usr/local/bin
	export PATH
	;;
esac

case "$PATH" in
*/usr/contrib/bin*)
	;;
*)	test -d /usr/contrib/bin && PATH=$PATH:/usr/contrib/bin
	export PATH
	;;
esac

case "$PATH" in
*/usr/bin/X11*)
	;;
*)	test -d /usr/bin/X11 && PATH=$PATH:/usr/bin/X11
	export PATH
	;;
esac

case "$PATH" in
*/usr/X11/bin*|X11*)
	;;
*)	test -d /usr/X11/bin && PATH=$PATH:/usr/X11/bin
	export PATH
	;;
esac

case "$PATH" in
*/usr/openwin/bin*|*X11*)
	;;
*)	test -d /usr/openwin/bin && PATH=$PATH:/usr/openwin/bin
	export PATH
	;;
esac

case "$PATH" in
*/usr/ucb*)
	;;
*)	test -d /usr/ucb && PATH=$PATH:/usr/ucb
	export PATH
	;;
esac

TTY=`tty`
case $TTY in
*cons*)		TTY=co				;;
*tty*)		TTY=`expr $TTY : '.*tty\(.*\)'`	;;
*/dev/*)	TTY=`expr $TTY : '.*/\(.*\)'`	;;
esac

case `id` in
uid=0\(*)
	ps1=" ${TTY}# "
	;;
uid=*)
	ps1=" ${TTY}% "
	;;
*)	if [ -w / ]
	then
		ps1=" ${TTY}# "
	else
		ps1=" ${TTY}% "
	fi
	;;
esac
export ps1
PS1="$_NODE$ps1"
export PS1
PS2='... '
export PS2

if [ -f /bin/mail ]
then
	mailprog=/bin/mail
elif [ -f /usr/bin/mail ]
then
	mailprog=/usr/bin/mail
else
	mailprog=mail
fi
export mailprog

case "$TERM" in
*xterm)	if [ -x /usr/openwin/bin/resize ]
	then
		resize(){
			eval `/usr/openwin/bin/resize`
		}
	elif [ -x /usr/bin/X11/resize ]
	then
		resize(){
			eval `/usr/bin/X11/resize`
		}
	elif [ -x /usr/X11/bin/resize ]
	then
		resize(){
			eval `/usr/X11/bin/resize`
		}
	elif [ -x /usr/bin/resize ]
	then
		resize(){
			eval `/usr/bin/resize`
		}
	else
		resize(){
			echo no resize!
		}
	fi
	eval `tset -s -Q`
	resize
	;;
esac

setps(){
	case "$1" in
	$HOME)	PS1="$_NODE$ps1"			;;
	/)	PS1="$_NODE /$ps1"			;;
	*)	PS1="$_NODE "`basename "$1"`"$ps1"	;;
	esac
	export PS1
	xname "$_NODE $1"
}

cdl(){
	dir=`$B/netpath -l $*`
	echo $dir
	cd $dir && setps ${dir}
}
cds(){
	dir=`$B/netpath -s $1 $MS/MSGS` 
	dir=`dirname $dir` 
	echo $dir
	cd $dir && setps ${dir}
}
cdx(){
	cd $1 && setps `pwd`
}
l(){
	ls -l $*
}
lb(){
	ls -l `$B/netpath -s $1 $MS/MSGS`
}
lc(){
	ls -C $*
}
lt(){
	ls -ltr $* | tail
}
lut(){
	ls -lutr $* | tail
}
lv(){
	${B}/netlink -v $*
}
l1(){
	${B}/netlink -vc1 $* | ${B}/netdis
}
l3(){
	${B}/netlink3 -vc1 $* | ${B}/netdis
}
mail(){
	case $# in
	0)	$mailprog	;;
	*)	case $1 in
		-f)	$mailprog $*	;;
		*)	${B}/netmail $*	;;
		esac		;;
	esac
}
na(){
	$B/netaddr -rv $*
}
ntn(){
	netparams -n | tr '/ ' '_-'
}
ss(){
	sf=`$B/netpath -s $1 $MS/MSGS`
	cat -v $sf | tail -2 | sed 1q
	ls -l $sf
}
start(){
	$B/netcontrol start $1
}
status(){
	$B/netcontrol status
}
tl(){
	tail -f $MV/${1-*}.log
}
tm(){
	tail -f ${1:-$S/made}
}
vn(){
	if test -x Bin/showparams; then Bin/showparams -n
	elif test -x _bin/netparams; then _bin/netparams -n
	else netparams -n
	fi | tr '/ ' '_-'
}
xname(){
	case "$TERM" in *xterm) $echo $n ']0;'${1:-$_HOST}''$c;; esac
}
