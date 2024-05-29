#!/bin/sh
#
#	Make a list of files for BINARY distribution.
#
SPOOLDIR=/usr/spool/MHSnet
eval `egrep 'SPOOLDIR' /usr/lib/MHSnetparams 2>/dev/null`
case `pwd` in
*_config)	cd ..		;;
*)		cd $SPOOLDIR	;;
esac
find . -type f -print | sed \
	-e '/BAD$/d' \
	-e '/\.back$/d' \
	-e '/\.lock$/d' \
	-e '/\.log$/d' \
	-e '/\.ol$/d' \
	-e '/\.ool$/d' \
	-e '/\.out$/d' \
	-e '/\/0=au/d' \
	-e '/\/OL.*/d' \
	-e '/\/[0-9]/d' \
	-e '/\/core$/d' \
	-e '/\/lock$/d' \
	-e '/\/log$/d' \
	-e '/\/tlog$/d' \
	-e '/\/olderlog$/d' \
	-e '/\/oldlog$/d' \
	-e '/^\.\/[^_]/d' \
	-e '/^\.\/_src\//d' \
	-e '/_call\/passwd/d' \
	-e '/_config\/.*\.brk/d' \
	-e '/_config\/.*\.lnk/d' \
	-e '/_config\/.*\.tmp/d' \
	-e '/_config\/commandsfile$/d' \
	-e '/_config\/initfile$/d' \
	-e '/_config\/postaddr$/d' \
	-e '/_config\/site\.details$/d' \
	-e '/_forward\/./d' \
	-e '/_lib\/initfile$/d' \
	-e '/_lib\/publicfiles$/d' \
	-e '/_lib\/printorigins$/d' \
	-e '/_lib\/privsfile$/d' \
	-e '/_lib\/netprivs$/d' \
	-e '/_lib\/MHSnetparams$/d' \
	-e '/_messages\/./d' \
	-e '/_route\/./d' \
	-e '/_setup/d' \
	-e '/_state\/.*file/d' \
	-e '/_state\/MSGS/d' \
	-e '/_state\/NOTES/d' \
	-e '/_state\/commandsfile/d' \
	-e '/_state\/promiscuous/d' \
	-e '/_state\/removed\.MSGS/d' \
	-e '/_state\/routefile/d' \
	-e '/_state\/statefile/d' \
	-e '/_stats\/Acc.*/d' \
	-e '/dead\.letter/d' \
>/tmp/tar.files

ls -l /tmp/tar.files
