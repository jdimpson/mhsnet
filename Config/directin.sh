#!/bin/sh
echo=/bin/echo
echo "type:	directin"				>>$link.lnk
echo "
        SUN-III is a predecessor to MHSnet which is less flexible and lacks
        many of MHSnet's features. It also lacks MHSnet's advanced protocols.
        Some sites are still running SUN-III; generally you will be told if
        this is so.
"
if answerno "Is this a connection to an ACSnet site with SUN III? "
then
	sed -e "/link filter:	filter43/d" ${link}.lnk >${link}.tmp
	mv ${link}.tmp ${link}.lnk
	shell=_lib/ttyshell
else
	echo 'link filter:	filter43' >>$link.lnk
	shell=_lib/NNshell
fi

echo "
If this is a new link you should add an entry to
/etc/passwd to create an account with:

	account name: $link
	home directory: $SPOOLDIR
	shell: $shell

You must give the account a password. This password
will be used by the remote machine to login.
"

case "$SYSTEM" in
sco_xenix)
	if answeryes "Is this a new link? "
	then
		echo "You should now create an MHSnet account with"
		echo "user name \"$link\"."
		echo
		echo "Don't forget to give the account a password. "
		echo "This password will be used by the remote machine to login."
		echo
		if answeryes "Do you want to make the account now? "
		then
			mkuser
		fi
	fi
	;;
esac

$echo $n "- hit return to continue -$c"
read x
