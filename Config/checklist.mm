.\" tbl | troff -mm
.nr Hy 0
.ds HF 3 3 2 2 2 2 2
.ds sN \s-1MHSnet\s0
.TL
\s+8MHSnet Port\s-8
.AF ""
.MT 4
.SA 1
.S 10
.FS \&
.nf
.in 0
\s6@(#)checklist.mm	1.21 05/12/16
.FE
.DS
.TS
expand box ;
lbp10w(6cm) cw(5cm) cw(5cm)
lbp10 | lbp10 c
lbp10 | lbp10 | lbp10
lbp10 | lbp8 | lbp8 .
Porting Site:
_
CPU type / MHz:	Op. Sys. Version:
_
Dist. Name:	Release:	Date:
_
Source Media:	density:	format:
_
Distribution media:	density:	format:
_
Device name:	benchmark:	CRC32:
.TE
.DE
.HU Preparation:
.VL "\w'\s14\(sq\s0XX'u"
.LI \s14\(sq\s0
Make distribution for port:
(enter your choices into the boxes above)
.BL
.LI \s14\(sq\s0
Choose format \(em tar / cpio?
.LI \s14\(sq\s0
Choose media \(em
\s-1QIC\s0 cartridge tape /
\(12" 9-track /
\s-1DAT\s0 cartridge /
\s-1EXA\s0byte cartridge?
.LI \s14\(sq\s0
Choose density?
.P
If cartridge tape, the density can be \s-1QIC\s0-11/24/120.
We can only do \s-1QIC\s0-120.
Use `mango' for \s-1QIC\s0-120 (\f(CW/dev/rmt0\fP).
(Most \s-1QIC\s0-150 drives can read \s-1QIC\s0-120, and vice-versa.)
.P
Use `karl' for \s-1DAT\s0 (4mm), `joyce' for \s-1EXA\s0byte (8mm).
.P
If \(12 inch tape, the density can be either 1600 bpi or 6250 bpi.
Use the drive on `beta' for both
\(em select the density on the drive
before using \f(CW/dev/rmt0\fP to write it.
.LI \s14\(sq\s0
Make the distribution.
.P
Use the shell script \f(CWMakeDist\fP in the source directory \f(CWDist\fP.
It takes one argument: a name for the port.
The script will prompt you for a fuller distribution name, and provide a release number,
(eg: \f(CW#123\fP).
(The name should include the cpu type and op.sys. version,
eg: \f(CWSPARC-4.1.1-static\fP,
but keep it short \(em it will appear in all logs when the network is running.)
Fill these in the box at the top (\fIDist. Name:\fP).
.P
\f(CWcd /usr/src/Sun4/Dist; MakeDist \fP\fIname\fP
.P
You can make the distribution tape using the shell script \f(CWMakeTape\fP,
eg: to make a \s-1DAT\s0 cartridge tape from the above distribution file:
.P
\f(CWcd /usr/src/Sun4/Dist; MakeTape \fP\fIname\fP \f(CWdat\fP
.P
An optional third argument changes the default blocksize.
.LE
.P
Take two copies if possible!
(One set may be defective.)
.LI \s14\(sq\s0
Take enough spare media to make the binary distribution,
and \fBbring back a copy of the compiled source\fP.
.LI \s14\(sq\s0
Take the porting log.
You should note down all exceptions that occur during the port.
.LI \s14\(sq\s0
Make sure the port has been confirmed,
and that the porting site has the necessary machine configurations prepared in advance
(not always possible,
but a common reason for time wasted is waiting around for hardware to be set up!)
Ask for all operating system / networking manuals to be made available.
.LE
.HU "At the porting site:"
.VL "\w'\s14\(sq\s0XX'u"
.LI \s14\(sq\s0
Create a source directory on a file-system with enough space.
The pathname chosen should end in ...`\f(CW/mhsnet/src\fP' as many of the `run' files
install the test distribution in `\f(CW../dist\fP'
The space depends on the architecture, and whether you select the
\f(CWcc -g\fP option or not.
A \s-1SPARC\s0 with \-g needs ~20Mb,
a \s-1VAX\s0 without \-g needs ~8Mb,
however \-g is preferred,
so that debugging is easier, if necessary later on.
.P
Extract the source from the media.
.LI \s14\(sq\s0
\f(CWAdmin/root.prof.sys5\fP is linked to `.profile', so `dot' it in.
This creates a set of defined variables and functions useful during the port and testing.
Try \f(CWset\ |\ more\fP.
.LI \s14\(sq\s0
Check the local operating system version
\(em look at the local manuals section 2.
Note the version in the box at the top.
.LI \s14\(sq\s0
There is a shell script \f(CWAdmin/checklib.sh\fP to find system calls,
user/group names, and the library containing the `socket' code:
.br
\f(CWsh Admin/checklib.sh\fP
.br
.LI \s14\(sq\s0
Select the user and group names \(em
`daemon' is the preferred option in both, but they should be privileged,
and not allow general access, except by privileged users.
.LI \s14\(sq\s0
Check the local mail programs used for delivery
\(em if \f(CW/usr/lib/sendmail\fP exists, use it,
almost none of the others have decent delivery options
(for rewriting the \f(CWFrom\fP address).
.LI \s14\(sq\s0
Investigate the local manuals setup.
Where are they installed?
How are they kept
\(em source / processed / packed / compressed?
Try:
.br
\f(CWman man\fP
.br
it may say where the manuals \fIought\fP to be installed.
(\f(CW/usr/bin/man\fP is often a shell script,
so you can look at it with \f(CWmore\fP,
or else use \f(CWstrings | grep man\fP.)
There are shell scripts in the `Manuals' directory to do the more common types of installation
(\fBafter\fP the binaries below).
Otherwise you will have to edit \f(CWManuals/Makefile\fP as appropriate.
The possibilities are:
.DS C
.TS
;
c c c
lfCW cfCW cfCW .
Manual Directory	MANTYPE	CONFIG
_
/usr/cat/cat?/*.?	T	stdcat
/usr/cat/man?/*.?	T	mancat
/usr/catman/?_man/cat?/*.?	T	sysvcat
/usr/catman/?_man/man?/*.?	T	sysvman
/usr/catman/g?m/*.?	T	gman
/usr/catman/man?/*.?	T	catman
/usr/catman/packages/man?/*.?	T	pkgman
/usr/catman/MHS/g?/*.?	T	ncrman
/usr/contrib/man?/*.?	T	contribman
/usr/contrib/man?/*.?.Z	TZ	hpman
/usr/man/cat.MHS/*.MHS	XENIX	scocat
/usr/man/cat?/*.?	T	stdman
/usr/man/catman/man?/*.?	T	sysvcatman
/usr/man/man.MHS/*.MHS.z	XENIX	scoman
/usr/man/man?/*.?	src	stdman
/usr/share/man/cat?/*.?	cat	sharecat
/usr/share/man/catman/man?/*.?	T	sharecatman
/usr/share/man/man?/*.?	src	shareman
.TE
.DE
.P
`\fBscoman\fP' is the \s-1SCO\s0 `packed' format.
Any of the others may have entries ending in `.z' (use \fBTz\fP)
or `.Z' (use \fBTZ\fP).
Some forms have the `.[1-8]' stripped off,
in which case append `s' to the \s-1MANTYPE\s0.
.P
If there are no system manuals,
install the post-nroffed versions
(use \s-1MANTYPE=T\s0).
.P
If both the
.I nroff
formatter and `cat' manual directories exist,
set \s-1MANTYPE\s0=cat.
.P
The point is that the setup script `installmhs' run by users
is just going to `ln [-s]' the files into the local manual directories
(if possible \(em otherwise it will use \f(CWcpio -p\fP).
.LI \s14\(sq\s0
Check \f(CWLib/Version.c\fP
\(em you may need to edit it to reflect the distribution more closely.
However, all the `run' files expect three environment parameters,
one of which,
.SM VERSION,
can be set instead of changing \f(CWVersion.c\fP:
.if n .ds tw 4
.if t .ds tw \w'\s-1ISPOOLDIR\s0X'u
.VL  "\*(tw"
.LI \s-1VERSION\s0
Sets up the macro for `run', default taken from \f(CWLib/Version.c\fP.
.LI \s-1IPSOOLDIR\s0
The pathname of the directory where the distribution will be built,
default: \f(CW/usr/spool/MHSnet\fP.
The best thing to do is choose some other directory where the distribution
can be built separate to testing.
Then fixes can be re-installed, and a new distribution made more easily.
.LI \s-1MANTYPE\s0
Choose one of \f(CWsrc\fP,
\f(CWT\fP (pre-nroffed),
\f(CWTZ\fP (pre-nroffed, compressed),
\f(CWTz\fP (pre-nroffed, packed),
\f(CWXENIX\fP,
\f(CWcat\fP (runs local nroff on local man macros),
or
\f(CWcat2\fP (runs local nroff on our man macros).
Defines what type of manuals are installed in
.SM ISPOOLDIR
when `run manuals' is invoked, default \f(CWT\fP.
.LE
.LI \s14\(sq\s0
Choose a `run' file from \f(CWMakefiles/run.*\fP as a template,
and copy it to \f(CWrun\fP.
Edit it to reflect the local environment
\(em note any variations in the porting log.
In particular,
edit the first few lines to specify the particular Op. Sys. / \s-1CPU\s0 versions.
Later, during the tests, you should send this file back to `basser' for future reference.
.LI \s14\(sq\s0
Note that the \s-1IGNMAILERSTATUS\s0 setting depends on type of mailer,
2 for sendmail (ignore \s-1EX_NOUSER\s0 returns),
1 for bozo mailers (ignore all error returns),
and 0 for /bin/mail (treat all errors as real errors).
.LI \s14\(sq\s0
Start the compilation:
.br
\f(CWrun; tm\fP
.br
`tm' is one of the shell functions set up by \f(CW.profile\fP that does:
.br
\f(CWtail\ -f\ $S/made\fP
.LI \s14\(sq\s0
Edit the install script that is going to be used by users in
\f(CWConfig/installmhs\fP.
Ensure the binaries go into the right place,
and the manuals type is selected appropriately.
Some `system' types are well known,
and you can select one of those to get appropriate defaults.
.LI \s14\(sq\s0
The default assumed by the software is always \f(CW/usr/spool/MHSnet\fP.
If there isn't enough room on \f(CW/usr/spool\fP to hold the installed binaries,
then you can create a symbolic link to another more spacious file-system first.
If symbolic links don't work,
set the value \f(CWISPOOLDIR\fP to someplace else.
.LI \s14\(sq\s0
You may now have time for coffee, but keep your eye on the `tail'.
If you get a chance, run \f(CWAdmin/bench.sh\fP
and note down the result under `benchmark' at the top.
.LI \s14\(sq\s0
When the `run' has finished successfully you can test the `crc' code:
.br
\f(CWrun check\fP.
.P
If there is an error,
you will have to look at the code in 
\f(CWLib/crc*.c\fP and fix it,
followed by a further `run'.
Note the \s-1CRC\s032 times under `CRC32' at the top.
.LI \s14\(sq\s0
Install the distribution:
.br
\f(CWrun directories special install manuals\fP.
.LI \s14\(sq\s0
If the above doesn't install the correct type of manuals,
you will need to install the manuals by hand
\(em either run one of the install scripts in \f(CWManuals\fP,
eg:
.br
\f(CWsh installcat\fP
.br
or, after editing \f(CWManuals/Makefile\fP, do:
.br
\f(CWcd Manuals; make directories install\fP
.P
The files ending in `T' are post-\fInroff\fP \(em
if \fInroff\fP doesn't exist, use \f(CWinstallT\fP to install these instead.
.LI \s14\(sq\s0
Install the config files:
.br
\f(CWrun config\fP.
.br
This has to come last
because it makes the file \f(CW_config/chmod_files\fP
and has to be able to \fIsee\fP \fBall\fP the installed files.
.P
This may fail with `\f(CWarg list too long\fP'
\(em probably because there are too many environment variables inherited from
\fImake\fP. So do it by hand:
.br
.ft CW
.ps -2
cd $ISPOOLDIR
.br
sh _config/make_files.sh NETUSERNAME NETGROUPNAME CHOWN SERVERUSER SERVERGROUP
.ps
.ft
.LI \s14\(sq\s0
Make a distribution `tar' file of the installed tree,
and then extract it into a new directory which will be used for testing,
preferably the default: \f(CW/usr/spool/MHSnet\fP.
.P
Test the installation:
.br
\f(CWcd $MC\fP
.P
Edit \f(CWconfigmhs\fP for the default modem device name, etc.,
then run the install script:
.br
\f(CW\&./installmhs\fP
.P
You can select `/tmp/bin' for the binaries,
but you really need to install the manuals properly
so that you can run the local `man' command to test them.
But first check you won't overwrite any local manuals!
(you may need to change the names of the \f(CW_man\fP manual entries if so).
.LI \s14\(sq\s0
Try \f(CWman netwhois\fP.
.LI \s14\(sq\s0
Test the configuration:
.br
\f(CWcd $MC; ./configmhs\fP
.P
This is also testing the program \f(CWmenu\fP
\(em if it doesn't set the screen up properly,
and screen programs are using the \fIcurses\fP(3) library
(ie: \s-1TERMCAP\s0 isn't defined, and \s-1SYSLIBS\s0 has \f(CW-lcurses\fP),
then you may need to add \s-1CURSES_TAB_BUG\s0
to the \s-1CONFIG\s0 variable in the \f(CWrun\fP file,
touch Config/menu.c
and remake and reinstall.
.P
For the site address,
use `test1.dep.co.oz.au',
the serial number `000000',
and the activation key \f(CWNDDDDAIL\fP.
.P
Make a link to `tmx.personis.oz.au',
login name `mhsnet',
password `\s-1SUN IV\s0',
telno \fB519 2044\fP
(remember to prefix with `\fBT\fP' or `\fBP\fP',
and the necessary code to get an outside line, usually `\fB0,\fP'),
speed 2400-38400 baud,
started \fIexplicitly\fP,
for a \fIdial-up\fP connection
using the link filter \fIpressfilter\fP.
(NB: check the availability of `compress' or `gzip' first,
and alter
.SM PATH
in `pressfilter.sh' if necessary.).
.P
There are alternate numbers listed at the end that you can use for a connection.
.LI \s14\(sq\s0
Use the appropriate menu option to start the network.
.P
This may produce errors in \f(CW_lib/start.log\fP.
You will need to fix the errors and try again.
If `netstate' complains on \s-1SYSTEM\s0 V.[012],
and \f(CW_state/MSGS\fP is owned by `root',
this is caused by a `mkdir' bug,
so activate the line containing
\f(CWsu daemon -c ...\fP in \f(CW_lib/start\fP
instead of the default.
.LI \s14\(sq\s0
Run the following tests:
.BL
.LI \s14\(sq\s0
\f(CWnetparams -n\fP
.P
It will show the version info. from `Lib/Version.c'.
.LI \s14\(sq\s0
\f(CWnetparams -f\fP
.P
It should show the free space available on the file-system containing
\s-1SPOOLDIR\s0
minus the amount specified in 
\s-1MINSPOOLFSFREE\s0
(normally 100Kb).
.LI \s14\(sq\s0
Test privileges:
.P
.nf
.ft CW
cd $ML
cp example.privs privsfile
netprivs jane
.ft
.fi
.P
It should show:
.br
.ft CW
jane: MULTICAST,EXPLICIT,OTHERHANDLERS,SEND,RECEIVE,EXEC @COUNTRY=au
.ft
.LI \s14\(sq\s0
Test network passwords:
.P
.nf
.ft CW
netpasswd tmx
.ft
.fi
.P
(use anything to mind) and then check it:
.P
.nf
.ft CW
netpasswd -c tmx
.ft
.fi
.P
and then print out the passwd file:
.P
.nf
.ft CW
netpasswd -p
.ft
.fi
.P
It should show something like:
.br
.ft CW
\^.9=tmx.3=mhs.2=oz.0=au:6-RfB3rfUkhQY:
.ft
.LI \s14\(sq\s0
\f(CWnetmap -rvm | more\fP
.P
It should show sensible routing tables.
.LI \s14\(sq\s0
\f(CWnetcontrol status\fP
.P
It should show `router' running,
three `admin' entries as `cronjobs',
and a line starting `call-tmx.personis.oz.au'.
.LI \s14\(sq\s0
\f(CWnetq -fax\fP
.P
There should be two state messages queued for `tmx'.
.LI \s14\(sq\s0
Try a local routing test:
.br
\f(CWnetmail root\fP
.br
and type a `.a' to get an acknowledgement.
.P
Check the messages have passed through the routing queue:
.br
\f(CWnetq -fax\fP
.br
(should look the same as above, ie: no messages in the \f(CW*errors*\fP queue).
.P
This should deliver two messages into root's mailbox.
Check that the header of the mail message is reasonable,
if not adjust the \s-1MAILERARGS\s0 parameter in \f(CW_params/mailer\fP
(which can go out on the final distribution).
Check that the header of the acknowledgement message is reasonable,
which is controlled by the \s-1BINMAILARGS\s0 parameter in \f(CW/usr/lib/MHSnetparams\fP.
Unfortunately, if this needs altering,
you will have to edit the `run' file and re-link the binaries.
.LI \s14\(sq\s0
Mail integration:
.P
Find out if it is easy to get the local mailer to pass messages to MHSnet
(no worries if it is `sendmail').
Make notes on the problems/solution.
.P
Use `netparams' to check the \s-1MAIL\s0 parameters:
.br
\f(CW(netparams -a; netparams -a mailer) | grep MAIL\fP
.LI \s14\(sq\s0
Check forwarding:
.br
\f(CWnetforward -U network root\fP
.P
Test that it worked:
.br
\f(CWnetmail network\fP
.P
Look in root's mailbox to see if it was delivered properly,
also check the directory \f(CW_forward\fP for modes etc.
.LI \s14\(sq\s0
Check that netget works:
.br
\f(CWnetfile -nc root $S/run -Ntest-run\fP
.P
Check that it gets created correctly:
.br
\f(CWnetget -csyUroot test-run\fP
.P
The new file should have the same size and mode as the original:
.br
\f(CWls -l $S/run test-run\fP
.LI \s14\(sq\s0
Put a file on the queue to tmx:
.br
\f(CWnetfile piers.cs.su.oz.au $S/run\fP
.P
Check that it got queued ok:
.br
\f(CWnetq -avAfiler tmx\fP
.LI \s14\(sq\s0
Check the
.I tty
connections:
.br
\f(CWnetcon\fP \fImodem\fP
.P
This will test \s-1RAW\s0 and \fIspeed\fP \fIioctl\fP system interfaces.
Try and get the modem to respond to the \f(CWATE1Q0\fP command.
.P
Some systems won't talk to a modem unless the modem asserts \s-1CARRIER\s0 first,
so either use a \fIbreakout\fP box to hardwire \s-1CARRIER\s0 up,
or program the modem while attached to a terminal.
The Interlink \s-1FASTBIT II\s0 needs the folllwoing command to force \s-1CARRIER\s0:
.br
\f(CWAT&C0\fP
.br
.ne 10
.P
Other useful \s-1FASTBIT II\s0 commands:
.DS CB
.ft CW
ATB14	# force V.32
AT$E0	# no MNP
AT$F3	# use RTS/CTS
.ft
.DE
.LI \s14\(sq\s0
Start a call:
.br
\f(CWnetcall tmx\fP
.P
This will show the progress of the call by tailing the file \f(CW_call/tmx.log\fP.
.P
After the connection is made,
hit \f(CWINTR\fP once to start \f(CWnetlink -vc1 | netdis\fP,
and watch the progress of message transfer.
.LI \s14\(sq\s0
The call will bring in some state messages from tmx,
so check that the routing tables now have details for `oz.au':
.br
\f(CWnetmap -r | more\fP
.P
and that tmx's message is in the data-base:
.br
\f(CWnetmap -v tmx\fP
.LI \s14\(sq\s0
Start an `admin' script:
.br
\f(CWnetcontrol start purge\fP
.P
This runs the `daily' script in \f(CW_lib/daily\fP.
Check `netinit's log in \f(CW_lib/log\fP to see that it worked ok.
.LI \s14\(sq\s0
If local \s-1TCP/IP\s0 is available,
make a distribution for another machine on the ethernet (as below),
install it,
and configure `test1.dep.co.oz.au',
serial number `000000',
activation key \f(CWNDDDDAIL\fP.
.P
Then make a \s-1TCP\s0 connection to `test1' from `test2',
and check that the socket code sets up ok.
Check that the correct entries are installed for `inetd',
and note the special cases for \s-1SCO UNIX\s0 (because of setluid(2)).
.P
If there is no `inetd' or equivalent, then you will have to enable `tcplisten'.
This is done by getting the \s-1TCP\s0 shell script in _config to issue the line
\f(CWtype: tcpin\fP
instead of
\f(CWtype: TCPin\fP
which causes `mkifile.awk' to activate `tcplisten' as a daemon in `initfile'.
.LI \s14\(sq\s0
Shut down the network:
.br
\f(CWnetwindup\fP
.P
Check that all processes terminated.
.LI \s14\(sq\s0
Check \fIroot\fP's mail (cf: \s-1NCC_ADMIN\s0) \(em
if there are any unexpected messages,
investigate them.
.LE
.LI \s14\(sq\s0
If the kernel is \s-1SYSTEM V\s0 with \f(CW/etc/rc?.d\fP directories,
test the shutdown/reboot scripts in \f(CW/etc/rc?.d/[SK]nnn.MHSnet\fP
(installed by \f(CWinstallmhs\fP).
Restart the network (use \f(CWnetstart\fP),
and then shutdown the system, and reboot it.
Check that the network shutdown cleanly,
and that it restarts on system boot.
.LI \s14\(sq\s0
Make the binary distribution,
probably on the same media on which you brought the source.
You can use the shell script \f(CW_config/cleanall.sh\fP
to clean out the temporary files created in the tests.
The distribution must be made in /usr/spool/MHSnet
(with pathname `.') so that it may be installed anywhere.
.LI \s14\(sq\s0
\fBWRITE-PROTECT THE DISTRIBUTION TAPE\fP.
.LI \s14\(sq\s0
Test the binary distribution:
go through an installation process in a new directory
to check everything up to and including the running of \f(CW_config/installmhs\fP
(which should create the file \f(CW/usr/lib/MHSnetparams\fP to rename the spool directory).
.LI \s14\(sq\s0
Note in the log the methods you went through to test the distribution,
so that we can prepare an customer installation guide for this distribution
(the exact `tar' command used, etc.)
.LI \s14\(sq\s0
Clean up.
.P
If you are not leaving a binary distribution at the porting site,
this will involve removing the spool directory \f(CW/usr/spool/MHSnet\fP,
and the parameter file \f(CW/usr/lib/MHSnetparams\fP (if it got created),
but you will also need to remove any manuals and binaries that got installed during the tests above.
.P
If you \fBare\fP leaving a binary distribution,
run \f(CW_config/cleanall.sh\fP.
.LI \s14\(sq\s0
Make a copy of the compiled source (for the name lists).
(This may not be possible on machines with floppy drives only!)
.LI \s14\(sq\s0
\fBREMOVE THE SOURCE!\fP
.LI \s14\(sq\s0
Check that all boxes have been ticked.
.LE
.ne 30
.HU "Miscellaneous:"
.DS CB
.TS
c s
l r .
Useful phone numbers
_
Piers Lauder	692 2824
Elaine Pensabene	550 4448
(BSDI telno	550 5014)
.sp
tmx V.\s-1FAST\s0	519 2221
tmx V.32bis	519 2044
tmx \s-1TRAILBLAZER\s0	557 5888
tmx cisco	550 3962
.sp
basser V.32bis	*692 9177
basser V.32bis	692 9134
basser V.32bis	692 9180
basser V.32bis	692 9183
basser V.32bis	692 9198
basser V.32bis	692 9254
basser V.32bis	692 9710
basser V.32bis	692 9989
.TE
.DE
.DS CB
.TS
c s s s s s
c c c c c c
l l l l l l .
Link names at basser/tmx
=
Link name	Login Name	Password	Shell	Filter	Key
_
test1	mhsnet	SUN IV	ttyshell	pressfilter	NDDDDAIL
test2	test2	SUN IV	NNshell	filter43	HGDLFLOH
test3	mhsnet	SUN IV	ttyshell	<none>	BFDMICMD
.TE
.DE
