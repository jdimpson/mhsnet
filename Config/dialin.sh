#!/bin/sh
echo=/bin/echo
. ./getaddr.tmp
echo "type:	dialin"				>>$link.lnk
echo "
        SUN-III is a predecessor to MHSnet which is less flexible and lacks
        many of MHSnet's features. It also lacks MHSnet's advanced protocols.
        Some sites are still running SUN-III; generally you will be told if
        this is so.
"
if answerno "Is this a connection from an ACSnet site with SUN III? "
then
	sed -e "/link filter:	filter43/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	shell=_lib/ttyshell
else
	echo 'link filter:	filter43' >>$link.lnk
	shell=_lib/NNshell
fi

if answeryes "Is this a new link? "
then
	echo "
You should now create an MHSnet account with
user name \"$link\".
"
	case "$SYSTEM" in
	sco_xenix)
		if answeryes "Do you want to make the account now? "
		then
			mkuser
		fi
		;;
	*)
		echo "
You should add an entry to /etc/passwd
to create an account with:

	account name: $link
	home directory: $SPOOLDIR
	shell: $shell
"
		;;
	esac

	echo "
Don't forget to give the account a password. This password
will be used by the remote machine to login. Alternatively,
you can set a \"network password\" for the account that will
be checked by $shell after login:
"
	if answeryes "Do you want to set a network password for them? "
	then
		if test -s ../_state/routefile
		then
			netpasswd $addr
		else
			echo "Please do \"netpasswd $addr\" after starting the network"
		fi
	fi
fi

$echo $n "- hit return to continue -$c"
read x
