.\" troff -mm
.\" OPTIONS mm
.\" SCCSID @(#)install.mm	1.10 93/11/27
.nr Hy 0
.ds HF 3 3 2 2 2 2 2
.ds sN \s-1MHS\s0net
.TL
Installation Guide for \*(sN
.AF "Message Handling Systems"
.AU "Piers Lauder"
.AU "(piers@mhs.oz)"
.AS
This document is a guide to the installation
of the \*(sN software from the distribution file.
It includes a detailed description for the configuration of the network.
Please read it with care,
and notify the author of any deficiencies, additions, or modifications
you might think are necessary.
.AE
.MT 4
.SA 1
.OF "''V E R S I O N   1.10'\\\\*(DT'"
.EF "'\\\\*(DT'V E R S I O N   1.10''"
.H 1 INTRODUCTION
The \*(sN software distribution arrives as either a
.I tar (1)
format, or
.I cpio (1)
format file.
The contents of this file should be extracted\*F
.FS
Extract a \fIcpio\fP format file with the command ``\f(CWcpio\ -icdm\ <file\fP''.
Extract a \fItar\fP format file with the command ``\f(CWtar\ xf\ file\fP''.
.FE
into an appropriately labelled directory on a file system
with at least ten megabytes of free space.
The basic source takes around one and a half megabytes,
the extra ones will only be needed during the installation process.
.P
There are many sub-directories created by the extraction process,
their contents will be explained below.
.P
There is only one file that you are going to have to edit
for the preliminary stage of compilation,
however,
there are other files that must be set up before the network will function,
so the best course of action is to make hard copies of all the manuals,
and the top-level
.I Makefile ,
and have them to hand during the installation process.
.H 2 "The name of your node."
Choosing the name of your node should be straightforward,
but there are a few rules you should bear in mind.
The name will consist of a set of
.I typed
domains,
of which one will be the machine name,
and another your country code.
The domain names must consist of letters from the set
.B [\-_A-Za-z0-9] ,
be preceded by their type (conventionally, but not necessarily, in upper-case)
and be concatenated together with periods (`\fB.\fP'),
as in
.DS 1
.ft CW
NODE=basser.DEPT=cs.ORG=su.PRMD=oz.COUNTRY=au
.ft
.DE
At least four domains should be present, more may be added:-
.VL "\w'\s-1COUNRTY\s0=XX'u+.25i" .25i
.LI \s-1COUNTRY\s0=
The country code, normally an internationally recognized two letter code.
.LI \s-1ADMD\s0=
The name of your ``Administrative Management Domain'',
normally controlled by one of the local PTTs.
This is unused in Australia at the moment.
.LI \s-1PRMD\s0=
The ``Private Management Domain''.
(``oz'' signifies
.I ACSnet .)
.LI \s-1ORG\s0=
The name of your ``Organization''.
.LI \s-1DEPT\s0=
(Or ``\s-1OU1\s0='', and ``\s-1OU2\s0='', ...)
Optional ``Organizational Units'', as needed.
.LI \s-1NODE\s0=
The name of the machine that is supporting the network locally.
.LE
.P
The concatenated domain names make up names of
.I regions
in the network.
The region consisting of all the domains except \s-1NODE\s0 is commonly referred to as your
.I site
address,
and should be used when referring to your network address,
in preference to the machine address
(eg: for mail addresses.)
.P
You may change the name of your node at any time.
You will need the cooperation of the managers at the nodes
that are directly connected to you, but the rest of the nodes
will find out about the change automatically,
as your new state information propagates through the network.
Be warned, though, that your users will object!
.H 2 "Installation should be easy."
You should read this document and follow the directions
as you go along,
then, by the time you reach the end of it,
you will also have the network up and running.
If it isn't, please let us know about any problems.
.H 1 "DIRECTORY HIERARCHY"
The directory tree created by the distribution is described below.
There are only three real files in the top level:
\f(CWBUGS\fP,
\f(CWREADME\fP,
and
\f(CWMakefile\fP.
The \f(CWBUGS\fP file contains a list of current known bugs
and suggested temporary fixes,
\f(CWREADME\fP tells you where to start, and announces any new features.
The fourth real file here will be the
.I run
file you create as part of the installation process.
The rest are all directories.
Where manuals have been provided,
they are referred to with the name in italics
followed by the section number in parentheses.
Some of the information contains unavoidable forward references
\(em for which, my apologies.
.H 2 Admin
This directory contains some shell programs to help administration that
can be run to start the network, and keep it tidy.
There are also useful files like a sendmail configuration
for 4.x \s-1BSD\s0 sites.
Glance through it when you get the time.
There are also the following administration commands:-
.VL "\w'checkmsgsdb.cXX'u+.25i" .25i
.LI dis.c
This is a management tool that can be used to turn
the repeated page output format of programs like
.I linkstats
into minimally updated screen displays.
See
.I dis (1).
.LI licence.c
Makes activation keys from licence numbers and site addresses.
.LI log.c
A program that can be used to bootstrap files over comms lines.
Like "cu", except that the file-transfer facility is reliable,
and it doesn't consult any control files.
.LI revpath.c
Management tool that turns addresses into pathnames,
and
.I vice-versa .
See
.I netpath (8).
.LI showcmds.c
Management tool for examining command files for messages in transit.
See
.I netcmds (8).
.LI showhandlers.c
Management tool for examining ``handlers'' validation file.
See
.I nethandlers (8).
.LI showhdr.c
Management tool for examining message files in transit.
See
.I nethdr (8).
.LI showparams.c
Management tool for examining the parameter files.
See
.I netparams (8).
.LI showprivs.c
Management tool for examining the privileges file.
See
.I netprivs (8).
.LI statspr.c
Prints selections from the statistics records produced by
the routing program.
See
.I netstatspr (8).
.LI test-crc.c
Installation tool for testing out new \s-1CRC\s0 implementations,
useful if you decide to write an assembler version for your new machine architecture.
Try
\f(CWrun test\fP
after you have created and installed
your new version of \f(CWLib/crc.c\fP
in \f(CWBin/Lib.a\fP.
.LE
.H 2 Bin
This directory is not in the distribution, but will be created when
you first run the program
.I make .
It will hold all the binaries made,
and also the libraries of sub-routines
common to many of the programs,
as well as site-specific header files.
.P
The contents of this directory are removed
by the command
\f(CWrun clobber\fP
(or
\f(CWmake clobber\fP)
which will also remove all the files produced during the compilation process.
A slightly less draconian clean-up can be produced by the command
\f(CWrun clean\fP,
which will leave the contents of
\f(CWBin\fP
alone, but remove all the compilation temporaries
(such as \fB.o\fP files).
.H 2 Config
Contains all the files that are installed in the spool directory
\f(CW_config\fP
that aid menu-driven configuration.
.H 2 Control
This contains the source of the programs used for network control functions:-
.VL "\w'checkmsgsdb.cXX'u+.25i" .25i
.LI address.c
Resolves network addresses.
See
.I netaddr (8).
.LI changestate.c
Invoked by the routing daemon to signal a change-of-state in the link.
Also used to scan for dead links.
See
.I netchange (8).
.LI checkmsgsdb.c
Compacts the messages data-base for broadcast messages.
See
.I netcheckdb (8).
.LI filter.c
A general purpose
.I filter
program to examine messages passing through a link
that is controlled by purpose-specific shell scripts.
See
.I netfilter (8).
.LI map.c
Allows users to interrogate the statefile and routing tables
to discover the network topology.
See
.I netmap (1).
.LI purge.c
Cleans out the work areas which can fill up with
timed-out messages, and discarded temporary files.
See
.I netpurge (8).
.LI request.c
Used to request a topology update from other network nodes.
See
.I netrequest (8).
.LI reroute.c
Manages re-routing of messages that are lost or delayed on broken links.
See
.I netreroute (8).
.LI state.c
The
.I statefile
manipulation program. Used by
.I changestate
and
.I stater
to update the statefile.
See
.I netstate (8).
.LE
.H 2 Dirlib
Contains a distribution of a \s-1UNIX\s0
version independent set of
directory reading routines.
See
.I directory (3).
.P
.ne 6
.H 2 Documents
This directory contains the following documents:-
.VL "\w'MHSnet.mmXX'u+.25i" .25i
.\" .LI design.mm
.\" A guide to the design of the software.
.\" .LI handle.mm
.\" A manual on how to write a message handler,
.\" and implement your own end-to-end protocols.
.LI install.mm
Installation guide.
Pre-process with
.I tbl (1).
.LI link.mm
A paper on ``Link Management and Link Protocol in \*(sN''.
.LI MHSnet.mm
A management guide for day-to-day running of the network.
There are also distributor-specific versions of this.
.\" .LI proto.ms
.\" A description of the transport protocol used by the message transport daemons.
.\" Pre-process with
.\" .I tbl (1).
.LI routing.mm
A paper on ``Broadcast Routing in \*(sN''.
.LI sun4.mm
``An overview of \*(sN''.
Pre-process with
.I pic (1).
.LE
.P
These are all written using the
.I mm (7)
or
.I ms (7)
macros for
.I troff (1).
Some of them require pre-processing with a combination of
.I pic (1),
.I eqn (1),
or
.I tbl (1),
as noted here, and in the first line of each document.
.H 2 FtHeader
Contains the source for the routines making up the library
\f(CWFtHeader.a\fP
in the directory
\f(CWBin\fP.
These routines manage the ``File Transfer Protocol'' headers.
.H 2 Handlers
These are all the network handlers, which are invoked to process
the contents of messages upon reception at a valid destination.
In particular, the program
.I router
is the one that makes all the routing decisions,
and invokes the other handlers.
.H 2 Header
Contains the source for the routines making up the library
\f(CWHeader.a\fP
in the directory
\f(CWBin\fP.
These routines manage the message headers.
.H 2 Include
All the
.I include
files that are used by programs.
.H 2 Init
The source for the network initialization programs.
See
.I netinit (8)
and
.I net (8).
.H 2 Lib
All the routines that are non-specific.
.H 2 Libc
Routines that ought to be in your standard C-library,
but may not be.
.H 2 Makefiles
This directory contains sub-makefiles
used by the main one in the directory above,
and also example \fIrun\fP commands for some standard configurations.
.P
\fIRun\fP commands pass all the configuration details to \fImake\fP
and prevent having to edit the \f(CWMakefile\fP itself.
They are \fIshell\fP scripts that invoke \fImake\fP
with an appropriate set of parameters.
If you have a standard configuration,
you should be able to choose one of these,
and avoid most of the editing stage described below.
If so, copy the appropriate one to the directory above,
and invoke it with the same parameters as you would for \fImake\fP.
.P
In any case, it is sensible to generate your own \fIrun\fP file
to match your current configuration,
as it makes adding updates and
porting to new systems much easier.
One of the existing \fIrun\fP files
should be used as a template
if there isn't a
.I run
file for your type of system.
Good starting points are
\f(CWrun.4.2\fP
or
\f(CWrun.4.3\fP
for Berkeley derived UNIX systems, or
\f(CWrun.sysV\fP
for UNIX System V.
The file
\f(CWrun.generic\fP
may also be helpful.
.P
\f(CWInstall\fP
and
\f(CWSite\fP
are just parts of
\f(CWMakefile\fP
that have been separated out to allow
.I make
to work on small machines without running out of heap space.
They are invoked directly from the main
.I Makefile
and you don't need to worry about them.
.H 2 Manuals
The manuals for sections 1 through 8
of volume 1 of the standard \s-1UNIX\s0 manual.
These should be copied into your user manual directories (\c
\f(CW/usr/man/*\fP
?).
Try editing the
.I Makefile ,
and typing
.ft CW
make install.
.ft
.H 2 Route
The routines that make up the library
\f(CWRoute.a\fP
in the directory
\f(CWBin\fP.
These routines access the
.I routefile
created by
.I netstate (1)
to perform routing functions.
.H 2 Utilities
These are the user programs for accessing the network:-
.VL "\w'fetchfile.cXX'u+.25i" .25i
.LI fetchfile.c
This is the rough inverse of
.I netfile (1).
It is used to request that another site
send you files that they have made publicly
available.
See
.I netfetch (1).
.LI forward.c
Control the message forwarding facility.
See
.I netforward (1).
.LI getfile.c
This program retrieves files received from the network.
See
.I netget (1).
.LI queue.c
A program used to show network queue status.
See
.I netq (1).
This program also fulfills the functions of stopping network messages
(see
.I netstop (1)),
and of manipulating dumped messages
(see
.I netbad (8)).
.LI sendfile.c
The program that pumps messages containing files into the network.
See
.I netfile (1).
The files are encapsulated in a
.I "File Transfer Protocol"
envelope, hereafter referred to as ``\s-1FTP\s0''.
.LI sendmail.c
The program that pumps messages containing mail into the network.
See
.I netmail (1).
The mail messages are passed to
.I netfile
for delivery.
.LI sendmsg.c
The program that pumps non-\s-1FTP\s0 messages into the network.
See
.I netmsg (8).
.LI whois.c
Used to query people data-bases at other sites.
See
.I netwho (1).
.LE
.H 2 VCdaemon
The source for the node-to-node virtual-circuit transport daemon.
See
.I netdaemon (8).
.I Linkstats
is a program also made from this directory,
which can be used to watch the state of a set of links.
See
.I netlink (1).
.H 2 VCsetup
The source for the programs that deal with intermittent links
over public access networks.
See
.I netcall (8)
and
.I netshell (8).
There is also a program called
.I tcplisten
for handling incoming \s-1TCP/IP\s0 calls
on systems that lack a \s-1BSD\s0 style
.I inetd .
See
.I netlisten (8).
.H 1 "EDITING THE CONFIGURATION FILE"
Your first job will be to edit the site-dependent parameters in an
appropriately named \fIrun\fP file in the directory
\f(CWMakefiles\fP.
This file should be linked to the name
\f(CWrun\fP
in the top-level directory.
You should check first that there isn't already an appropriate
.I run
file in the
.I Makefiles
directory \(em if not, use another similar one as a template.
You should choose one based on your kind of UNIX system.
No other files in the distribution should need to be changed by you!
If this proves not to be the case,
please let me have details of the deficiency/bug as soon as possible.
.P
A printed copy of
\f(CWMakefile\fP
is a useful companion when creating your
.I run
file.
Each of these parameters is exported from the
.I run
file into a couple of site-dependent
header files called either
\f(CWInclude/site.h\fP
(for numeric parameters), or
\f(CW$(BIN)/strings-data.h\fP
(for strings).
.P
Most of the parameters should be defined.
If they are numeric in type,
then their effect can be turned off by giving them the value \fB0\fP.
Some parameters that have
.I string
values can safely be undefined by just deleting their definition from the
.I run
file.
The text should make it all clear.
.P
The parameters are listed in the same order as you will find them in
\f(CWMakefile\fP.
Note that where these parameters appear in the rest of the document,
you should interpolate their values.
.H 2 "System Interfaces"
The first set of parameters are to do with the network's interface
to your kernel and to the file system, and to programs invoked by the network.
Some parameters specify the names of programs,
and the format of any special arguments
that will need to be passed to them on invocation.
There are some macros that can be used inside arguments
to interpolate fields from the message being processed by the handler,
they are all identified by the special character \fB&\fP:-
.P
.VL "\w'&%XX'u+.25i" .25i
.LI &D
.I "Data name" .
The name of the data in the message.*
.LI &F
.I From .
The name of the sender of the message.*
.LI &H
.I Home .
The name of the destination host (on which the program is being invoked).
.LI &L
.I Link .
The name of the link on which the message arrived.
.LI &O
.I Origin .
The name of the host from which the message originated.
.LI &T
.I "Start time" .
The time that the message was inserted into the network at its origin.
.LI &U
.I User .
The name of the user to which the message is being delivered.*
.LI &&
If you really need to interpolate just an \fB&\fP.
.LI &%
The way to escape a \fB%\fP (see below).
.LE
.P
Those marked with an asterisk are only valid for messages containing \s-1FTP\s0.\*F
.FS
Messages containing ``File Transfer Protocol'' are generated by
.I netfile (1).
.FE
.P
There is another set of macros that can be used to interpolate a string
depending on the evaluation of a simple boolean expression.
If the string is not interpolated the macro is replaced by the null string,
and if this causes the entire argument to become empty it is
omitted from the list of arguments.
These macros are all enclosed by the special character \fB%\fP, and
take the form:-
.P
.DS 1
\fB%\fP[\fB!\fP]\fIX\fPstring\fB%\fP
.DE
.P
If the condition selected by \fIX\fP evaluates to \fBtrue\fP
and the optional \fB!\fP was not given
then \fIstring\fP is interpolated.
Where the \fB!\fP is given \fIstring\fP is interpolated
when the condition evaluates to \fBfalse\fP.
The \fIstring\fP given may contain \fB&\fP substitutions,
however nested \fB%\fP substitutions are not permitted.
.P
Selector characters (\fIX\fP) are:-
.P
.VL "\w'&%XX'u+.25i" .25i
.LI B
True if the message had a
.I broadcast
address.
.LI L
True if \fB&O\fP is not identical with \fB&L\fP,
eg: if the message did not originate at the neighbour node.
.LE
.P
Arguments should be specified just as though you were listing them inside
a C program function invocation,
except that you must escape the double quotes to prevent them being interpreted by the shell.
Eg: to pass three arguments to a program,
the first interpolating ``-I'' if the message was not broadcast,
the second the name of the sender, and the third the name of her host,
you might specify:-
.P
.DS 1
.ft CW
ARGS = \e"%!B-I%\e",\e"-u&F\e",\e"-h&O\e"
.ft
.DE
.P
Note the use of double-quotes.
.P
Those parameters following that are marked with an asterisk (`*')
may be altered after configuration \(em see
.I netparams (5).
.P
.S -2
.VL "\w'\s-1IGNMAILERSTATUS\s0xx'u"
.LI \s-1BINMAIL\s0*
This defines the name of the program that will be used to deliver
mail generated internally by the network, eg: acknowledgements of
correctly delivered files, or error reports to the
.I NCC_MANAGER .
It must be able to read the body of the mail text from its standard input,
and to accept many user addresses as arguments
(after any special arguments mentioned in \s-1BINMAILARGS\s0 below).
\f(CW/bin/mail\fP
should always work.
.LI \s-1BINMAILARGS\s0*
If you have a version of mail that understands about interpolating the
origin host address in the mail item, then this is where you tell it what to do.
At Basser, where we use a version of mail developed locally,
we use the argument ``\fB\-n\fP\fIuser\fP\fB@\fP\fIhost\fP''
to tell mail that we wish the item to appear to come
from someone other than the invoker;
ie: we set \s-1BINMAILARGS\s0 to be \f(CW"-n&F@&O"\fP.
.LI \s-1FILESERVERLOG\s0*
The name for an optional file which will be used to log fileserver requests.
Typically this would be \f(CWSPOOLDIR/LIBDIR/servedfiles\fP.
.LI \s-1FILESEXPIREDAYS\s0*
After files have been spooled at a site for delivery to a local user,
the user is notified by mail that the files have arrived,
and warned that the spooled files will be deleted
after a certain number of days.
This is where you specify that period.
It is effected by a periodic command run by
.I cron (1),
see the section
.I Maintenance
for more details.
.LI \s-1IGNMAILERSTATUS\s0*
Some delivery programs, notably
\f(CWsendmail\fP
from 4\s-1BSD\s0,
return non-zero exit status, even when they have done something sensible
with the mail item.
In this case, the handler would normally assume that something went wrong,
and return the mail item to its origin with an appropriate error message,
but you can suppress this action by defining this flag to be \fB1\fP.
In which case, you must be perfectly sure that your delivery program will
always do the right thing!
.LI \s-1INSBIN\s0
The directory where user visible network programs are to be installed.
We use
\f(CW/usr/bin\fP,
on some systems
\f(CW/usr/local\fP
might be more appropriate.
.LI \s-1LOCALSEND\s0*
If you wish to allow users to send files to others on the same
node without doing a permission check.
.LI \s-1LOCAL_NODES\s0*
If you wish to allow users to send files to
a restricted number of sites, such as within
one Department, without doing a permission check, this define
should name a file where the sites should be listed one per line.
Alternatively, it can just be a valid network address.
.LI \s-1MAILER\s0*
This defines a program that will be used to deliver mail
received off the network, ie: from users at other hosts.
It must behave in the same way as \s-1BINMAIL\s0 above,
and again,
\f(CW/bin/mail\fP
should always work.
However, if you have a special program used just for
.I delivering
mail, then this is the place to specify it.
.LI \s-1MAILERARGS\s0*
If \s-1MAILER\s0 can be told where the mail originated,
then it should be specified in this argument,
much the same as for \s-1BINMAILARGS\s0 above.
Please note that this is the only safe way to guarantee that
mail ``From'' lines are valid.
\s-1MAILER\s0 should interpolate this string in place of
any user/site addresses in the body of the mail item.
For \s-1BSD\s0 \f(CWsendmail\fP use:
.DS 1
.ft CW
MAILERARGS=\e"-em\e",\e"-oit\e",\e"-f&F@&O\e",\e"-oMs&O\e",\e"-oML&L\e",\e"-oMrSunIV\e"
.ft
.DE
and use
\f(CWAdmin/sendmail.cf\fP
as your
\f(CW/usr/lib/sendmail.cf\fP
(or incorporate the changes into it).
.LI \s-1MAIL_FROM\s0*
If your version of \s-1MAILER\s0 does not correct the ``From'' line
using the information from \s-1MAILERARGS\s0,
then setting this to \fB1\fP
will enable the mail handler itself
to add a standard \s-1UNIX\s0 mail header.
.LI \s-1MAIL_RFC822_HDR\s0*
Setting this to \fB1\fP causes internally generated mail to include an
RFC822 compliant mail header.
.LI \s-1MAIL_TO\s0*
Setting this to \fB1\fP causes internally generated mail to include a
``To:\ ''
header line.
.LI \s-1MAXRETSIZE\s0
Limits the maximum size of a returned message.
Should not be less than about 2000
(to allow the header and sub-headers to escape mutilation),
but we recommend that everyone set this to 10000.
.LI \s-1MAX_MESG_DATA\s0
Limits the maximum size of a ``message part''.
Messages larger than this are split into parts transmitted separately
(unless the destination is directly connected.)
Should probably be around 2Mb,
but is really dependent on the minimum size of all intermediate spool areas
that any message might travel through.
We recommend that everyone set this to 2097152L
(the `L' is for C-compilers on 16-bit \s-1CPU\s0s.)
.LI \s-1NCC_ADMIN\s0*
This defines the name of the person to whom network generated error reports are sent.
By preference a local network wizard,
maybe
``Postmaster'',
or possibly
``root'',
alternatively it may be the network address of
someone central who fixes problems
in your branch of the network.
This is most useful when you have a large network
with central control.
.LI \s-1NCC_MANAGER\s0*
The same kind of address as \s-1NCC_ADMIN\s0 above,
only this is for non-urgent network information messages.
The same defaults as for \s-1NCC_ADMIN\s0 above.
.LI \s-1NETADMIN\s0*
Set to \fB1\fP to cause mail notices of new regions added to routing tables
to be sent to \s-1NCC_MANAGER\s0.
Also causes notification of new/deceased links to your site.
Set it to \fB2\fP for mail notices every time a new site sends its first state message
(there are a lot of sites out there ...)
.LI \s-1NETGROUPNAME\s0*
This parameter,
together with \s-1NETUSERNAME\s0,
is used to define the account which the network will operate under.
They ought not to be
.I root ,
but should probably be recognized by the mail and possibly news programs
as a privileged originator of mail.
.LI \s-1NETPARAMS\s0
This is the name of a file where changes to the default configuration parameters may be specified.
It must be in some publicly accessible directory,
\f(CW/usr/lib/MHSnetparams\fP
would be a good choice.
This file doesn't have to exist,
unless you wish to change some of the parameters you are specifying here.
.LI \s-1NEWSARGS\s0*
Any arguments for \s-1NEWSEDITOR\s0.
.LI \s-1NEWSCMDS\s0*
This should be the name of a file which is used by
.I reporter
to provide the full pathname of a news command and any extra arguments.
This can safely be ignored until you become a sophisticated news gatherer,
but could be \f(CWSPOOLDIR/LIBDIR/newscmds\fP.
The file should contain lines of the form:
.P
.DS 1
.ft CW
command<tab>full_path_name[<space>arg ...]
.ft
.DE
You should consult with your news neighbours to determine
the necessary commands to install here (if any).
.LI \s-1NEWSEDITOR\s0*
If you participate in the
.I netnews
system,
this argument specifies the news receiver program to be invoked by the handler
.I reporter .
For the standard netnews software,
this should be set to
.ft CW
/usr/bin/rnews.
.ft
For the news system distributed by U.N.S.W.,
this should be set to
.ft CW
/usr/lib/postnews.
.ft
.LI \s-1NICEDAEMON\s0*
If you wish the transport daemons to run at a non-standard priority,
then this parameter should be defined as for use in the
.I nice (2)
system call.
For instance, if you wanted the network to run only when there was
idle \s-1CPU\s0 time, then give it the value 19.
However, be aware that if you do this when the system is busy,
then the links may appear to go up and down a lot.
On the other hand,
we use a value of -11 to ensure the transport daemons always achieve high throughput,
which is acceptable since they are not very \s-1CPU\s0 intensive.
A value of 0 is probably safe.
.LI \s-1NICEHANDLERS\s0*
When a message arrives,
and is passed to the routing program to be processed,
this parameter will cause the handlers to be run at a different priority
from the daemon.
We use a value of 10, which is nice to users.
This parameter may also be specified on a per-handler basis by altering the
appropriate field in the
.I handlers
file (see
.I nethandlers (5).)
Note that the router processes messages serially,
so slowing down a particular handler will also delay any following messages.
.LI \s-1NODENAMEFILE\s0*
For systems without the \s-1SYSTEM V\s0
.I uname (2)
system call, ot the \s-1BSD\s0 4.x
.I gethostname (2)
system call, this parameter nominates a file containing the nodename.
.LI \s-1POSTMASTER\s0*
This must be set to be a name that messages returned from your node
may legally be sent to in the case of the origin also returning them.
If your mail program doesn't recognize anything special such as
\s-1POSTMASTER\s0, then this must be the name of a real user.
International networking conventions require that
``postmaster''
be a valid mail address on your system as a way to
guarantee deliveries of enquiries and bug reports.
\s-1POSTMASTER\s0 would be set to
``postmaster''
in this case.
.LI \s-1PRINTER\s0*
The name of the program used to spool print jobs from the network.
Leave it undefined if this host has no printer,
or if you have no intention of letting other sites print their jobs here.
(But see \s-1PRINTORIGINS\s0 below.)
.LI \s-1PRINTERARGS\s0*
Any arguments for \s-1PRINTER\s0.
.LI \s-1PRINTORIGINS\s0*
If you \fBdo\fP define \s-1PRINTER\s0, then you will need to restrict
the sites from which you are prepared to accept print jobs.
This parameter can be either the full path name of a file
containing a list of legal network origins for print jobs
(which is preferable for site independent network binaries),
or just a list of sites specified as a multicast address.
Use C strings in both cases.
Eg: a file \(em \f(CWSPOOLDIR/LIBDIR/printorigins\fP,
or a site list \(em \f(CW*.cs.su.oz,metro.ucc.su.oz\fP.
If it is undefined, then the print handler will reject all jobs.
.LI \s-1PRIVSFILE\s0*
The name of a file where network user's privileges will be listed.
\f(CWSPOOLDIR/LIBDIR/privsfile\fP is the usual choice.
(see
.I netprivs (5).)
.LI \s-1PROTO_STATS\s0
This forces the inclusion of code for collecting packet statistics
in the protocol routines.
If there,
then the message transport daemons print out the information on termination.
.LI \s-1PUBLICFILES\s0*
If you wish to allow others to retrieve files from
your system with the
.I fetchfile
command, then you should define this.
It can be set to the name of a directory
that will contain the publicly available
files (and directories containing publicly
available files and directories).
An example might be \f(CWSPOOLDIR/$(WORKFLAG)public\fP.
Alternatively, you can define
.I \s-1PUBLICFILES\s0
to be the name of a file that contains more
specific details of precisely who may access
which files.
Typically this would be \f(CWSPOOLDIR/LIBDIR/publicfiles\fP.
This file contains lines of the following form
.DS 1
.ft CW
[<user>@]<address><tab>[full_path_name]
.ft
.DE
.ds St \fB\v'0.1v'*\v'-0.1v'\fP
.ds Us <\fIuser\fP>
.ds Ho <\fIaddress\fP>
Either \*(Us or \*(Ho may be \*(St,
which implies any user (or address).
A missing \*(Us implies
any user at the specified address (as does \*(St.)
.I Address
may be any valid network address,
such as \f(CW*.anu.oz\fP,
or \f(CWbasser,cluster\fP.
It will be expanded to canonical form
before being matched against the source address of the request.
See the section on ``\s-1ACTING AS A FILESERVER NODE\s0''.
.LI \s-1SERVERUSER\s0*
If
.I \s-1PUBLICFILES\s0
is defined, then you may further restrict access
by defining
.I \s-1SERVERUSER\s0
to be a login name provided by your
system precisely for this purpose.
That is, you should create a new user name
in your password file, typically called
.I fileserver ,
who is prevented from logging in, and who
never owns any files.
If you already have such a user for other
purposes, using it should be fine.
.I \s-1SERVERUSER\s0
should be defined to be this user name.\*F
.S
.FS
Note: some systems limit the number of characters
in a login name (typically to 8).
This restriction is not important here, as
that restriction relates to the
.I login
program, which this particular user
should never use.
However, it might be wise to observe the
restriction if it does exist, as other
system utilities, such as
.I ls ,
have been known to make similar assumptions.
.FE
.S
.I Fileserver
will set its
.I uid
to be that of this user before accessing any files,
so only those files that this special user has normal
\s-1UNIX\s0 access permissions to read will be
able to be accessed.
.P
It is \fBstrongly\fP recommended that if you
have defined
.I \s-1PUBLICFILES\s0
then you also create such a user, and
define
.I \s-1SERVERUSER\s0 ,
otherwise
.I fileserver
will run as
.I \s-1NETUSERNAME\s0 ,
which is probably not a good idea.\*F
.S
.FS
While the security checks in
.I fileserver
are tight, there are no guarantees
that they are impregnable.
.FE
.S
.LI \s-1SERVERGROUP\s0*
This provides a facility similar to
.I \s-1SERVERUSER\s0 ,
with the obvious difference that
the name defined should be the name
of a newly created group, and is
used to set the
.I gid
of fileserver.
Note: \s-1AUSAS\s0 systems\*F
.S
.FS
See \fISystem Dependencies\fP below.
.FE
.S
do not use groups in the same way, and this
definition is not used.
Instead, the group defined in the
password file for
.I \s-1SERVERUSER\s0
is used.
.P
Usually, a group called
.I fileserver
will be created for this purpose.
If
.I \s-1SERVERGROUP\s0
is not defined, and this is not an \s-1AUSAS\s0 system,
.I fileserver
will run with the group id
.I \s-1NETGROUPNAME\s0 .
.LI \s-1SPOOLDIR\s0*
This should be the name of a directory
where all the network activity will take place.
It will contain all the messages that are
.I "in flight"
as well as the handler programs and network state files.
It should be somewhere safe,
where the integrity of the messages is least likely to be compromised,
(so try and avoid less than fully reliable disk drives.)
The usual choice would be
\f(CW/usr/spool/net\fP.
.LI \s-1STATERNOTLIST\s0*
This names a file that can be edited to contain the addresses
of regions whose state messages you wish ignored.
Useful for network backbone sites acting as major gateways.
.LI \s-1SUN3\s0
Set to \fB1\fP if there are any sites on the net
running the previous version of the software.
If you also wish to run a gateway to the previous version
then you should also set the following \s-1SUN3\s0 parameters.
Look in your \s-1SUN3\s0 source (or \fIrun\fP file) for the setting for these parameters.
.LI \s-1SUN3LIBDIR\s0*
If you are also running the previous version of the software,
then this string should be set to the name of the \s-1SUN3\s0 library spool directory,
normally
\f(CW_lib\fP.
.LI \s-1SUN3SPOOLDIR\s0*
If you are also running the previous version of the software,
then this string should be set to the name of the \s-1SUN3\s0 spool directory,
normally
\f(CW/usr/spool/ACSnet\fP.
.LI \s-1SUN3USERNAME\s0*
If you are also running the previous version of the software,
then this string should be set to the username
that \s-1SUN3\s0 programs operate under,
normally
``daemon''.
Note that if this is different from \s-1NETUSERNAME\s0 above,
then gateway programs will have to operate
.I setuid
to
.I root
in order to change the ownership of messages crossing the gateway.
We strongly recommend that \s-1NETUSERNAME\s0 be the same as \s-1SUN3USERNAME\s0.
.LI \s-1SUN3WORKDIR\s0*
If you are also running the previous version of the software,
then this string should be set to the name of the \s-1SUN3\s0 work file directory,
normally
\f(CW_work\fP.
.LI \s-1TMPDIR\s0*
The name of a directory where very temporary files are created.
We use \f(CW/tmp\fP.
.LI \s-1UUCPLOCKS\s0*
\fB1\fP if you run
.I cu ,
.I tip
or
.I uucp
and you want compatible locking on devices.
.LI \s-1VALIDATEMAIL\s0*
Some mail delivery programs are guaranteed to
fail\*F
.S
.FS
Dump a
.I core
file, or write a
.I dead.letter
file.
.FE
.S
if the destination user doesn't exist, in which case the handler
might as well check first that the user exists in the password file.
You can force this check to happen by defining this flag to be \fB1\fP.
.LI \s-1VCDAEMON_STATS\s0
This forces the inclusion of code for collecting throughput and kernel statistics
in the message transport daemons.
If there,
then the message transport daemons print out the information on termination.
.LI \s-1VMS\s0
Set to \fB1\fP if you want to use the \s-1VMS\s0 gateway.
.LI \s-1VMS_NET\s0
If you are connected to sites running the \s-1VMS\s0 operating system,
and have decided to use the mail interface code supplied in the directory
\s-1VMS\s0,
then this parameter should be set to be the path name of a spooling program
that will accept messages from the Sun4_VMS gateway handler.
.LI \s-1WHOISARGS\s0*
Any additional arguments you might want to pass to \s-1WHOISPROG\s0.
.LI \s-1WHOISFILE\s0*
The name of the people data-base.
It should contain all the information about your users
that is deemed publicly accessible.
A carefully pruned copy of
\f(CW/etc/passwd\fP
would do, but you should convert all capitals to lower-case,
unless your version of
\s-1WHOISPROG\s0 has an ``ignore case'' flag,
(in which case turn it on in \s-1WHOISARGS\s0 above).
For example,
you could use the following command
(perhaps run once a month by
.I cron\c
) to make the pruned
.I whois
file:
.DS 1
.ft CW
cut -d: -f1,5 /etc/passwd | tr '[A-Z]' '[a-z]' >/usr/pub/whois
.ft
.DE
.LI \s-1WHOISPROG\s0*
The name of the program which will attempt to match the supplied pattern
in the
.I whois
data-base.
It will be passed arguments as for
.I egrep (1),
which is the obvious candidate.
.LI \s-1WORKFLAG\s0
This parameter should be set to a single character that will distinguish
.I work
directories from
.I link
directories in \s-1SPOOLDIR\s0.
Some of the directories in \s-1SPOOLDIR\s0
are the names of directly connected nodes.
These
.I link
directories contain command files
describing messages queued for that particular link.
This flag is used to distinguish these directories from the work directories
for the benefit of programs such as
.I linkstats
which must search \s-1SPOOLDIR\s0 for the names of links.
The usual choice would be \fB\(ru\fP (rule, or underbar).
Any non-\c
.I alpha-numeric
character will do,
but take care to choose one that is usable with the shell and
.I make .\*F
.S
.FS
For instance, some versions of
.I make
object to finding the character `\fB=\fP' in a name
which is to appear in a rule as a dependency of a target.
.FE
.S
.LE
.S
.H 2 "System Dependencies"
The next set of parameters are to do with the peculiarities of your
particular version of \s-1UNIX\s0.
Some of these are inter-dependent on most systems,
but may be isolated additions on others.
The software is written assuming a basic AT&T Level 7 \s-1UNIX\s0 system,
and these parameters allow for extra
.I features
provided by later versions.
They all take numeric values,\*F
.FS
Except for \s-1BSD4V\s0 and \s-1SYSLIBS\s0.
.FE
and will be used as arguments in C pre-processor \fB#if\fP lines,
so set them to \fB0\fP to nullify them.
.P
.S -2
.VL "\w'\s-1IGNMAILERSTATUS\s0xx'u"
.LI \s-1AUSAS\s0
Refers to your system having the features of the
.I "Australian \s-1UNIX\s0 Share Accounting System" .
This flag makes the network cognizant of the different
.I passwd
file accessing routines provided by this system.
A value of \fB1\fP turns it on.
.LI \s-1AUTO_LOCKING\s0
Also a part of the \s-1AUSAS\s0 package,
a value of \fB1\fP means that the
.I auto-lock
file type exists.
.LI \s-1BSD4\s0
This flag should have the value of the first number after the decimal point
of your Berkeley 4th. distribution. eg: 3 for 4.3\s-1BSD\s0.\*F
.S
.FS
If you have 4.1z\s-1BSD\s0, define this parameter as for 4.2\s-1BSD\s0.
If you have 4.0\s-1BSD\s0 (really?),
setting this parameter to 0 will do all the right things.
.FE
.S
.LI \s-1BSD4V\s0
If you have the beta-test-site distribution of 4.1c\s-1BSD\s0,
set this flag to be \fBc\fP.
.LI \s-1CARR_ON_BUG\s0
\fB1\fP if you have \s-1OPENDIAL\s0=1 and broken
device drivers,
which only set the software copy of the modem
signal \s-1DCD\s0 (\s-1CARR_ON\s0) on ``open'' calls,
and not on ``ioctl'' calls.
This is true for most Unisoft \s-1SYSTEM III\s0 systems,
eg: Integrated Solutions (DE Unity).
.LI \s-1FCNTL\s0
\fB1\fP if your system has the
.I fcntl (2)
system call
(ie: not 4.1? \s-1BSD\s0).
.LI \s-1FLOCK\s0
\fB1\fP if your system has the \s-1BSD\s0
.I flock (2)
system call.
.LI \s-1KILL_0\s0
\fB1\fP if a signal value of 0 can be passed legally to the system call
.I kill (2)
in order to interrogate for the existence of a process.
True in \s-1SYSTEM III/V\s0, and in 4.2\s-1BSD\s0.
.LI \s-1LOCKF\s0
\fB1\fP if your system has the
.I /usr/group
standard locking.
If you have a \s-1SYSTEM V\s0 system with file locking primitives (check
\f(CW/usr/include/fcntl.h\fP)
do \fBnot\fP define \s-1LOCKF\s0 to \fB1\fP.
.LI \s-1MINSLEEP\s0
\fB1\fP on almost every version of \s-1UNIX\s0 except Version 7,
where it should be \fB2\fP.
The problem there was that the kernel sometimes treated an alarm call for 1 second
as infinite, due to race conditions.
.LI \s-1NDIRLIB\s0
\fB1\fP if your system is a non-\s-1BSD\s0 kernel but has the \s-1BSD\s0
portable directory reading library routines installed in \fIlibc.a\fP.
\fB2\fP if your system is a non-\s-1BSD\s0/non-\s-1SYSTEM V.3\s0 kernel
but has the \s-1SYSTEM V.3\s0
portable directory reading library routines installed in \fIlibc.a\fP.
[But note, if ``sys/dirent.h'' doesn't exist, set it to 0.]
.LI \s-1PGRP\s0
\fB1\fP if your kernel supports the
.I setpgrp (2)
system call as per \s-1SYSTEM V\s0.
\fBNot\fP true for 4.2\s-1BSD\s0.
.LI \s-1RENAME_2\s0
\fB1\fP if your kernel has the
.I rename (2)
system call.
.LI \s-1SYS_LOCKING\s0
\fB1\fP if your kernel has the Unisoft
.I locking (2)
system call.
.LI \s-1SYSLIBS\s0
Should be set to any extra system libraries that need to be searched by
.I ld (1).
.LI \s-1SYSTEM\s0
Set this flag to be the version of the AT&T \s-1SYSTEM\s0 that you are running,
\fB0\fP otherwise.
Eg: \fB3\fP for \s-1SYSTEM III\s0, \fB5\fP for \s-1SYSTEM V\s0, etc.
.LI \s-1SYSVREL\s0
Set this to be the \s-1SYSTEM V\s0 release version.
.LI \s-1TCP_IP\s0
\fB1\fP if your system has TCP/IP libraries and
.I sockets .
.LI \s-1UDP_IP\s0
\fB1\fP if your system has UDP/IP libraries and
.I sockets .
.LI \s-1V8\s0
\fB1\fP if your kernel is \s-1V8\s0 (or \s-1V9\s0).
.LI \s-1VPRINTF\s0
\fB1\fP if your C-library includes the
.I vprintf (3)
routine (just makes ``varargs'' processing a bit faster.)
.LI \s-1WAIT_RESTART\s0
\fB1\fP if your kernel has the feature that the
.I wait (2)
system call is automatically restarted after a caught signal
if there is an active child process.
This is true for all BSD4.3 systems.
.LE
.S
.H 2 "Kernel Parameters"
Some kernel magic numbers are in non-standard places on all versions
of \s-1UNIX\s0, so here is where you get to dig them out.
.P
.S -2
.VL "\w'\s-1IGNMAILERSTATUS\s0xx'u"
.LI \s-1FILEBUFFERSIZE\s0
This should be set to the size of the buffers in the file system buffer cache.
Note that this is not necessarily the same as
\s-1BUFSIZ\s0 in the include file
.I stdio.h .
``512'' should be adequate,
but if buffers are 8K in your system,
then setting this appropriately will improve throughput.
.LE
.S
.H 2 "User Visible Program Names"
You can choose what to call the programs that are going to be part
of the user interface to the network.
The sensible names to use are the ones in the manual entries as distributed.
However this section is for those of
you with an aesthetic sensibility appalled by the official choices.
Remember that if you do change the names, then you should also
fix up the manual entries to reflect your new choices.
.P
Two names in particular are
.I known
to the network internally,
and if you change them after configuration,
then you must also set the new values into the relevant parameter files
(see
.I netparams (5).)
They are \s-1GETFILE\s0, and \s-1SENDFILE\s0.
.H 2 "Compilation Control"
This section should be familiar to people who have already grappled
with the
.I make
program.
There are two sets.
.P
\s-1CFLAGS\s0 and \s-1LDFLAGS\s0 should be set to be appropriate
options for your C-compiler and link-loader.
\fB\-O\fP for the C-optimizer and \fB\-n\fP for shared-text from the loader
should be generally applicable.\*F
.FS
If your \s-1CPU\s0 is a \s-1PDP\s0-11 with
.I "separate I & D space" ,
specify the \fB\-i\fP loader flag also.
.FE
\s-1DEBUG\s0 can be null, or can be set to be either
.ft CW
\-DEBUG=1
.ft
which will include some error detection and code for issuing
.I reports ,
or to be
.ft CW
\-DEBUG=2
.ft
which will enable yet more code for doing
.I traces
(which will enable all the programs to produce trace output when the
\fB\-T\fP flag is used). Eg:
.DS 1
.ft CW
.ta +3i
DEBUG = -DEBUG=1	# provide reports
.ft
.DE
.P
\s-1CONFIG\s0 can be used on the command line for
.I make
to pump
in circumstantial parameters.
If your \s-1CPU\s0 is a Motorola 68000 series,
and the C compiler pre-processor doesn't define the name
.I mc68000
then \s-1CONFIG\s0 should be set as follows:
.DS 1
.ft CW
.ta +3i
CONFIG = -Dmc68000	# define the CPU type
.ft
.DE
If you do have a Motorola 68000 series CPU,
then you also need to define the assembler type
in the one and only file in the distribution
containing assembler code,
\f(CWLib/crc.c\fP.
The problem is that there are several flavours of assembler in use
for these chips, so you will have to see what you've got, and choose
something appropriate. You may even have to add code for your particular
system \(em if so, please let me have a copy for inclusion in the next release.
So far, all the flavours of assembler outside Bell Laboratories seem to use
an assembler syntax compatible with the \s-1MIT_68K\s0 version.
So the two choices are
.DS 1
.ft CW
.ta +3i
CONFIG = -Dmc68000 -DMIT_68K	# define the assembler type
.ft
.DE
or
.DS 1
.ft CW
.ta +3i
CONFIG = -Dmc68000 -DBELL_68K	# define the assembler type
.ft
.DE
If your \s-1CPU\s0 is a Microvax, you should set \s-1CONFIG\s0 as follows:
.DS 1
.ft CW
.ta +3i
CONFIG = -Duvax		# define the CPU type
.ft
.DE
so that the \s-1VAX\s0 ``crc'' instruction assembler code that the Microvax does not support
will not be included.
.P
Some C compilers don't really support \f(CWvoid *\fP properly,
in which case you need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DNO_VOID_STAR	# don't let cc divide by 0!
.ft
.DE
.P
Some C compilers don't support \f(CWvoid\fP at all,
and these should have:
.DS 1
.ft CW
.ta +3i
CONFIG = -DNO_VOID	# ancient cc
.ft
.DE
which also sets \s-1NO_VOID_STAR\s0.
.P
Some C compilers don't like the use of constants in switch statements
unless they are explicit integers, in which case you will need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DINT_SWITCH	# fussy cc
.ft
.DE
.P
Similarly, some C compilers don't like the use of enumerated types in expressions
unless they are cast to integers, in which case you will need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DENUM_NOT_INT	# broken cc
.ft
.DE
.P
If you have selected either one of \s-1TCP_IP\s0 or \s-1UDP_IP\s0 above,
and your kernel is \s-1SYSTEM V\s0 with the same
.I socket
include files as a standard BSD system,
then you need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DBSD_IP
.ft
.DE
.P
The second set are the names of programs used by
.I make
to do its work.
For instance,
.I chown (1)
and
.I mknod (1)
often seem to live in peculiar places.
The only complex choice is the specification of the program
that will create a locking file (in \s-1MKLOCK\s0).
On \s-1AUSAS\s0 systems, this is done with the program
.I mknod (2)
and a file type of \fBa\fP,
which is specified in the parameter \s-1MKLOCKTYPE\s0.
On all other systems,
\s-1MKLOCK\s0 should be set to be something like
.I touch (1)
which will just create an ordinary file,
and the parameter \s-1MKLOCKTYPE\s0 should be null.
The actual locking is then taken care of by code conditional
on the \s-1UNIX\s0 version parameters.
.P
If a program doesn't exist,
or you wish to cancel its effect,
use a variation on one of the following assignments:-
.P
.DS 1
.ft CW
.ta +3i
STRIP = :	# do nothing
STRIP = : strip	# show what you are not doing
.ft
.DE
.P
The exceptions to this rule are the programs for pre-sorting libraries,
and the program for making libraries randomly accessible.
If the programs
.I lorder (1)
and
.I tsort (1)
don't exist then you should cancel them as follows:-
.P
.DS 1
.ft CW
.ta +3i
LORDER = echo	# just echo the arguments
TSORT = cat	# just copy input to output
.ft
.DE
.P
If the program
.I ranlib (1)
doesn't exist (\s-1SYSTEM III\s0 or early \s-1SYSTEM V\s0)
then it should be cancelled as for \s-1STRIP\s0 above.
If the system doesn't have a loader that automatically produces
randomly accessible libraries, then one or other of these library
sorting programs must exist,
and on BSD loaders sometimes insist on libraries being processed with
.I ranlib.
.P
On most systems, the library routine
.I malloc (3)
is very slow,
and in order to speed up the routing calculation by up to 60%,
it is worth while using a routine that calls
.I brk (2)
directly:
.DS 1
.ft CW
.ta +3i
CONFIG = -DSLOW_MALLOC
.ft
.DE
.P
However, on some systems,
such as the DECstation 3100,
and Sun Micro's \s-1SPARC\s0, the
.I malloc (3)
library has been strangely modified,
to the extent that the routing ``Malloc'' won't work unless the
memory boundaries are aligned on some large number of bytes,
so you should either not define \s-1SLOW_MALLOC\s0, or add:
.DS 1
.ft CW
.ta +3i
CONFIG = -DALIGN=512
.ft
.DE
(which will, of course, lead to a large memory hog!)
.P
The system call
.I fsync (2)
exists on all \s-1BSD\s0 4.2+ systems,
is rumoured to exist on \s-1SYSTEM\s0 V.4,
but may also be present on some \s-1SYSTEM\s0 V.3 systems,
in which case you need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DFSYNC
.ft
.DE
.P
Some implementtaions of Berkeley-style sockets
have the structure \f(CWiovec\fP defined as \f(CWuiovec\fP,
in which case ytou need:
.DS 1
.ft CW
.ta +3i
CONFIG = -DUIOVEC
.ft
.DE
.H 1 "SYSTEM BUGS"
In this section we discuss a few well known system bugs that may
plague your installation.
Not all systems have them,
but if they exist they will all
cause \fBmake\fP to fail in one way or another.
.P
If
.I ranlib
does not exist on your system,
be warned that not all versions of
.I lorder
and
.I tsort
do a proper sort;
if you get undefined global errors at
.I ld (1)
time,
you will need to re-arrange the library concerned by hand with judicious
applications of the command
\f(CWar ma ...\fP
(see
.I ar (1)).
.P
On the other hand,
there is a well known bug in
.I ranlib
when the number of symbols in the existing library
\f(CW_\|_.SYMDEF\fP entry
just happen to make the first word \f(CW==\ 0408\fP.
.I Ranlib
will fail with a message about ``old fashioned archive format'',
and the only solution is to remove the offending library and try again.
.P
On small machines,
(with small address spaces),
.I make
may fail with the message
``too many env parameters''
\(em try stripping the environment as in
\f(CWenv - run\fP.
.P
The Gould system manual suggests that
\f(CWkill(pid,\ 0)\fP
is legal \(em but the Gould kernel disagrees.
You should define \s-1KILL_0\s0 to be \fB0\fP for these machines.
.P
If you are running one of Melbourne University's versions of 4.2 BSD,
.I make
may fail because \fIfd_set\fP is defined in both \f(CWInclude/global.h\fP
and in \f(CW/usr/include/sys/types.h\fP.
In that case, just take out the \fItypedef\fP in \f(CWInclude/global.h\fP.
.P
A similar problem will affect you if any of the other \fItypedef\fPs
in \f(CWInclude/global.h\fP (such as the one for \fIUlong\fP)
have been made elsewhere
(\f(CW/usr/include/sys/types.h\fP is a favourite place).
The solution is the same as for \fIfd_set\fP
\(em just take out the one in \f(CWInclude/global.h\fP.
.P
Some of the source assumes that the library function \fIbcopy\fP is safe
if the arguments overlap. In particular it assumes that \fIbcopy\fP can handle
copying to overlapping lower addresses (as in the C version in Libc/bcopy.c).
If your C-library version doesn't, please arrange to use the one provided.
.H 1 "MAKEFILE TARGETS"
Now we can get serious.
The default
.I make
target,
``all'',
will make the
.I Bin
directory,
set up the
.I site.h
and
.I strings-data.h
files,
and then make the libraries followed by all the programs.
.P
Try it. Type
.DS 1
.ft CW
run
.ft
.DE
.P
If you need to re-do anything, then just type
.ft CW
run
.ft
again.
.P
Finally, the command
.DS 1
.ft CW
run\ directories\ special\ install
.ft
.DE
will set up all the work directories,
create some network data-base files,
and install all the binaries.
The actual
.I makefile
involved with implementing these targets is called
\f(CWMakefiles/Install\fP.
Read it if you want.
Then do it.
.P
If you wish to make a listing at any stage, then try
.DS 1
.ft CW
run PRFORMATTER=pr PRSPOOL=lpr print
.ft
.DE
or some variation thereof.\*F
.FS
The last time we tried this,
it used slightly under 1200 pages of 66 line paper.
.FE
.H 1 REGIONS
A brief discussion of regions and their configuration is necessary.
Regions are hierarchical name spaces in the network.
They consist of \fItyped\fP domain names,
each domain naming some entity in the network,
such as an organization, or a computer (node.)
The region name for a particular organization
is generally referred to as its \fIsite address\fP.
There may be many nodes,
or even organizational sub-units (eg: \fIdepartments\fP) containing nodes,
within any one site.
One of the reasons for the introduction of regions is to reduce the
number of sites that must be remembered in the topology database.
A lot less information is needed to remember the route to a region,
than to remember the routes to all the members of that region.
This is brought about if all those nodes which are essentially local
work-stations are relegated to site specific regions.
Knowledge of the local nodes will not appear in the databases of
nodes at other sites,
yet they may still be addressed explicitly by prepending the node name
to the site address (which \fBis\fP known.)
There is another advantage \(em you may choose node names that aren't
unique across the whole network,
yet which become unique when the site address is included.
Nodes are just special cases of typed domain names,
so domain names themselves also conform to this rule,
ie:- domain names become unique in a wider context
by appending the name of an encompassing region.
.P
Domain names are typed,
and so different types may share the same name.
Eg: \f(CWNODE=basser\fP and \f(CWDEPT=basser\fP are two different domains.
.P
There are a few rules to consider when configuring domains for a node.
The name of a domain (or node) must be unique within its region.
Every node must declare its
.I "visible region"
which essentially defines the largest region within which the node
will propagate its routing information.
If the visible region chosen is larger than necessary,
then information about the node will appear in more places than necessary,
and will incur unnecessary overheads for other nodes.
However,
another important property of the visible region
is that no messages originating outside it,
and whose destination addresses lie outside it,
will pass through the node.
So if you wish to allow messages from other sites to traverse yours,
then the visible region should also encompass those sites.
.P
The ``private management domain'' encompassing all of the sites running \*(sN
in Australia is called ``oz''.
So, if your node is linked to other nodes outside your site,
your visible region should probably be ``\s-1PRMD\s0=oz.\s-1COUNTRY\s0=au''.
.P
In our case,
our local domain is called ``\s-1DEPT\s0=cs''
(for ``Department of Computer Science'').
We are part of Sydney University,
for which we have chosen the domain name ``\s-1ORG\s0=su''.
We have nodes that are local to the department whose
visible region is therefore ``\s-1DEPT\s0=cs.\s-1ORG\s0=su.\s-1PRMD\s0=oz.\s-1COUNTRY\s0=au'',
However,
the main nodes communicate with other organizations,
and so their visible region is ``\s-1PRMD\s0=oz.\s-1COUNTRY\s0=au'',
In fact,
all sites in Australia should have an address ending in ``.\s-1COUNTRY\s0=au'',
or, to put it more generally,
the domain denoting your country should always be one of the domains
mentioned in the address.
(Note that because the domains are
.I typed
they don't have to appear in any particular order,
although the convention is left to right in order of increasing significance.)
.H 1 "CONFIGURING THE COMMANDSFILE"
You must now create the file
\f(CWSPOOLDIR/STATEDIR/commandsfile\fP
to control the behaviour of the program
.I netstate (1).
There is an example ``commandsfile'' in the
.I Admin
directory, that you might like to look at first.
.P
The
.I netstate
commands that you put in
.I commandsfile
give you total control over the information that appears in your
network
.I state
and
.I routing
tables.
You can give a description of your node that will be seen by all
other sites, control the choices made by the routing algorithm,
suppress or encourage the knowledge of particular links that
will be seen by the rest of the world, and tell the network
how to handle any special gateways that you are connected to.
You should now read the
.I netstate (1)
manual, a couple of times.
.P
Here is a sample
.I commandsfile
from the node ``basser''.
It should help you master most of the possibilities.
It is split into fragments, with comments after each one.
Note that fields in each command are separated by
.I tabs
or
.I spaces .
They may also be separated by a 
.I newline
followed by a leading
.I space
or
.I tab
(as in 
.I make (1).
.br
.ne 10
.H 2 "The local node"
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Configure the preferred types:
#
ordertypes	COUNTRY;ADMD;PRMD;ORG;DEPT;NODE
#
#	Shorthand types:
#
map	C	COUNTRY
map	A	ADMD
map	P	PRMD
map	O	ORG
map	D	DEPT
map	N	NODE
.ft
.DE
.S
.P
First thing to notice is that the character \fB#\fP introduces a comment
extending to the end of a line, and blank lines are ignored.
Next, the
.I ordertypes
command lists our preferred type-names in order of decreasing significance.
These names will be used when exporting type names for region addresses,
and replace the defaults, which are terse, but boring.
There should be at least 5, up to a maximum of 10.
The first is for the country identifier,
the last is always assumed to be for a network node.
Note that you must always quote some version of the first 4 (together with the last),
even though some may not be in use (eg: \s-1ADMD\s0.)
.P
We then define some shorthand names for the types,
because we wish to save some typing in the following commands
where long type names could become tedious.
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Map in some real names:
#
map	ACSnet	P=oz
map	Australia	C=au
map	Austria	C=as
map	CompSci	D=cs
map	Computer\e Science	D=cs
map	New Zealand	C=nz
map	Switzerland	C=ch
map	SydUni	O=su
map	Sydney\e University	O=su
.ft
.DE
.S
.P
Here we map some real names for the commonly used typed domains.
In particular, note that our department's name is
``cs''
(for Computer Science),
the organization is
``su''
(for Sydney University),
the private management domain is
``oz''
(for ACSnet),
and the country is
``au''
(the international 2-letter code for Australia.)
Note that spaces are legal inside names, provided they are escaped.
We will use some of these names in the following commands,
just because they are prettier than the typed names
(which could be used instead.)
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Our address:
#
address	N=basser.CompSci.SydUni.ACSnet.Australia
#
#	Only visible within ACSnet:
#
visible	N=basser.CompSci.SydUni.ACSnet.Australia
	ACSnet.Australia
.ft
.DE
.S
.P
First we use the domain names to define the
.I "region address"
of the node we are configuring.
The node name is
``basser'',
and the remaining domains are selected from those defined above.
The second command defines the region within which
.I basser
will be able to resolve addresses,
and within which it will propagate its routing information.
Also, other sites will not attempt to route messages through
``basser''
that are destined outside our visible region.
Note that the full region address must be used,
and that commands may be continued onto successive lines
by preceding each successor with a leading space or tab.
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Site details:
#
organization	Sydney University Computer Science
postal_address	Department of Computer Science\e
	Madsen Building, F09\e
	Sydney University\e
	N.S.W. 2006\e
	Australia
system	MIPS M120, UMIPS 3.10
location	33 53 25.0 S / 151 11 18.7 E
remarks	Teaching
person	Ray Loyzaga
email_address	yar@cs.su.oz.au
telno	+61-2-692-3423
.ft
.DE
.S
.P
The site details are for the information of other sites on the network.
Two of the commands are mandatory,
.I organization
(it may be spelt with an `s')
and
.I telno ,
but the other details should also be provided for the benefit of site surveys,
network mapping attempts, and management aids.
The contact
.I person ,
.I "email_address"
and
.I telno
are all for someone who can be contacted to deal with network problems,
(general enquiries about people etc. should be directed to
``\f(CWpostmaster@\fP\fIsite_address\fP''.)
Note that the
.I "email_address"
provided is the ``site'' address
.I "sans types" ,
ie: the \s-1NODE\s0 domain is omitted.
We wish to discourage the use of nodenames in mail addresses,
as nodes are ephemeral things.
Note that
.I newlines
may be escaped by preceding them with a back-slash,
so that the
.I "postal_address"
will contain actual newlines.
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Map an easier handle:
#
map	R	N=basser
.ft
.DE
.S
.P
This is just an
.I alias
for the node, for the convenience of poor typists.
It is a local name only, but can be used to refer to
the local node in addresses, as in:\- \f(CWnetmap -v r\fP
(case is not significant.)
.H 2 Links
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Configure the links:
#
add	N=atom.O=lhrl.ACSnet.Australia
link	N=basser.CompSci.SydUni.ACSnet.Australia,
	N=atom.O=lhrl.ACSnet.Australia
linkname	N=atom.O=lhrl.ACSnet.Australia
	atom
spooler	N=atom.O=lhrl.ACSnet.Australia
	SPOOLDIR/LIBDIR/Sun4_3
.ft
.DE
.S
.P
This is an example of a link into ``basser''.
``lhrl'' is an outside organization still running the old software.
We first
.I add
the node's address, then make a bi-directional link to it,
(uni-directional links are made with the
.I halflink
command.)
Note that links are specified by a comma separated pair of node addresses.
Then we give the link a short directory name for ease of use.
The default would be a directory name constructed from the reverse ordered type-less address.
Finally we specify a
.I spooler
to handle the gateway to the other network software.
(The other network also should be configured to gateway messages to \*(sN,
by specifying a
.I spooler
such as \f(CWSPOOLDIR/LIBDIR/Sun3_4\fP.)
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
add	N=research.O=btl.P=com.C=us
link	N=research.O=btl.P=com.C=us,
	N=basser.CompSci.SydUni.ACSnet.Australia
delay	N=research.O=btl.P=com.C=us,
	N=basser.CompSci.SydUni.ACSnet.Australia
	43200
restrict	N=research.O=btl.P=com.C=us,
	N=basser.CompSci.SydUni.ACSnet.Australia
	ACSnet.Australia
map	research.usa
	N=research.O=btl.P=com.C=us
.ft
.DE
.S
.P
The second link example is to a site running the same software.
They call us every twelve hours,
so we set the maximum queueing delay for messages to be the same (in seconds.)
We wish to receive only Australian ACSnet traffic over this link,
so a
.I restriction
is placed on the link into ``basser''
limiting traffic from ``research''
to messages addressed to sites within ``ACSnet.Australia''.
(They have a similar restriction on their end limiting traffic from us
to messages addressed to sites within ``O=btl.P=com.C=us''.)
Finally we map an old address for this site into the real one,
for the convenience of users.
.H 2 "Address Forwarding"
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Forward all messages addressed explicitly
#	to the department, Sydney University or ACSnet:
#
forward	CompSci.SydUni.ACSnet.Australia
	N=cluster.CompSci.SydUni.ACSnet.Australia
forward	SydUni.ACSnet.Australia
	N=metro.D=ucc.SydUni.ACSnet.Australia
forward	ACSnet.Australia
	N=munnari.D=cs.O=mu.ACSnet.Australia
.ft
.DE
.S
.P
The
.I forward
command allows us to pass messages arriving at our node addressed to sites or even countries
to some naming authority with a greater chance of resolving them.
So we forward messages addressed just to the department to ``cluster'',
which has the largest people database for the department.
Messages addressed to the University are forwarded to the Computing Centre,
and messages addressed to ACSnet itself are forwarded to Melbourne University,
which is also the international gateway for ACSnet.
.H 1 "SETTING UP AUTO-CALL FACILITIES"
.I "\*(sN"
provides a facility for making dynamic connections between nodes
over
.I dial-up
lines using
.I auto-call
units,
or via virtual circuits established on Public Packet Switched Networks.
If there are to be any nodes that you wish to call automatically
(whenever the queue becomes non-empty, and there is not already a
transport daemon running), then a couple of things must be set up.
.P
The
.I auto-call
facility is provided by the program
.I netcall (8).
This program in turn provides support for running auto-call
.I programs
written in an appropriate interpreted language
to control the establishment of a virtual circuit to some remote
node over some combination of intervening networks.
The successful completion of the program will result in
two transport daemons communicating with each other
over the virtual circuit.
.I Netcall
expects at least one argument, which is the name of a program to run.
The routing program knows how to invoke
.I netcall
with a program named ``call''
if the link is suitably flagged,
and there is indeed such a file name
in the link's queueing directory.
As an alternative to
.I netcall
you can specify a different call program
using the
.I netstate
command
.I caller .
.P
You must create a call program
to make the connection and start the daemon.
There are several examples in the
.I netcall
distribution directory.
Put a copy of the program in the link's queue directory
in a file labelled
.I call .
Then add a line to
.I commandsfile
to let the routing program know:-
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Sydney UniversityXX'u
#
#	Flag the auto-call links:
#
flag	N=basser.CompSci.SydUni.ACSnet.Australia,
	N=research.O=btl.P=com.C=us
	call
.ft
.DE
.S
.P
Alternatively, if other nodes are going to call you,
then you need to create a
.I login
name for them to connect to.\*F
.FS
The method of doing this is idiosyncratic to your brand of \s-1UNIX\s0,
but on most versions it consists of using the editor
to add an extra line to the file
\fI/etc/passwd\fP.
.FE
The normal practice is for the account name to be the same
as the name of the node that is calling in,
but you can also choose any alias (map) for that node
that you might have set up in
.I commandsfile
(including the directory names specified by the
.I linkname
command.)
The
.I shell
to be invoked must be set to be the path name of an appropriate installed version of
.I netshell (8).
(Versions of
.I netshell
exist for many types of virtual circuit.)
It is also advisable to set a password
for the account.
Some versions of
.I login (1)
(eg in 4\s-1BSD\s0 and \s-1V\s0\&7),
restrict the length of a login name allowed.
If yours does this and the name of one of the nodes dialing in is too long,
then you can add an alias that is shorter,
and
.I netshell
will look it up to find the real node name.
Example:
.P
.DS 1
.ft CW
gris::64:1:Softway Network Login:SPOOLDIR/LIBDIR/ttyshell
.ft
.DE
.P
(Don't forget to add a password.)
.H 1 "CONFIGURING NETINIT"
The day to day operation of the network involves running many different programs. 
Some programs are to be run continuously,
such as the routing program or 
message transport daemons talking to permanently connected systems. 
These programs must be restarted if, for some reason, they terminate.
Other programs are run at particular times of the day, week or month.
Dialup calls to other systems may be made several times a day 
at particular times.
Management programs that clean up directories and produce statistics need
to be run once a week or once a month.
.P
Rather than require the system administrator to run and restart these programs the 
.I netinit
program is provided.
This program runs programs,
restarts them if necessary and can run them at nominated times.
.P
The description of what programs to run and when is found in the 
file \f(CWSPOOLDIR/LIBDIR/initfile\fP.
Each program that is to be run by
.I netinit
has an entry of the form:
.P
.DS 1
\fIprocess-id\ process-type\ command\fP
.DE
.P
The 
.I process-id 
starts at the beginning of a line and is delimited by space, tab or newline.
It is used as an identifier for the process when
.I netinit
is asked to start or stop it by the 
.I net (8)
program (described later).
It would normally be a word that describes the process to be run, for
example ``call-cluster'' might be the
.I "process id"
for the program that
makes a call to machine ``cluster''.
.P
The
.I process-type 
is either the word
.B restart ,
the word 
.B runonce ,
or a specification of the time to run the process in the same form as that used by
.I cron (8),
enclosed in double quotes.
.P
The 
.I command 
specifies the program to execute along with its arguments. 
.P
Lines beginning with white space are interpreted as continuations of the
current entry. An empty line terminates an entry and a line beginning with 
non-white space signifies the beginning of a new entry.
Comments begin with a \fB#\fP character and end at a newline.
.P
.I Netinit
changes directory to \f(CWSPOOLDIR\fP before starting,
so pathnames can be relative to \f(CWSPOOLDIR\fP,
or absolute.
.H 2 "Permanently Running Programs"
Some network programs need to be run continuously and restarted if they stop.
The routing program,
\f(CWSPOOLDIR/LIBDIR/router\fP
needs to be active at all times to process routing requests.
.P
.I Netinit
can be told to run and restart router by placing a line such as:
.P
.DS 1
.ft CW
.ta +\w'call-munnariXX'u +\w'"0 0,12,18 * * *"XX'u
router	restart	LIBDIR/router -f
.ft
.DE
in
\f(CWSPOOLDIR/LIBDIR/initfile\fP
(the flag \fB-f\fP prevents \fIrouter\fP from forking off a child to do the work.)
.H 2 "Controlling Netinit"
.I Netinit
can be asked to start and stop processes or groups of processes,
re-read \f(CWinitfile\fP,
report the status of processes or shutdown all processes in an orderly way using the 
.I netcontrol
program.
.P
.I Netcontrol
takes a command name argument and an optional regular expression,
and carries out the requested operation on all processes
whose process names match the regular expression.
See
.I netcontrol (8)
for further details.
.H 1 "SETTING UP PERIODIC CALLS"
If you only (or also) wish to make
.I periodic
calls to some other node,
then you need to add an
.I initfile
entry to make the calls at the chosen intervals.
For instance,
to call the node ``munnari'' at noon, 6pm, and midnight,
place appropriate commands in a shell_script
\f(CWSPOOLDIR/_call/call_munnari\fP,
and add the line:-
.P
.DS 1
.ft CW
.ta +\w'call-munnariXX'u +\w'"0 0,12,18 * * *"XX'u
call-munnari	"0 0,12,18 * * *"	_call/call_munnari
.ft
.DE
to
\f(CWSPOOLDIR/LIBDIR/initfile\fP.
.P
As another example,
if we had a Hayes compatible dialup modem and wished to call
``cluster'' at 7am and 7pm,
then we could add to
\f(CWSPOOLDIR/LIBDIR/initfile\fP
the line:
.P
.DS 1
.ft CW
.ta +\w'call-munnariXX'u +\w'"0 0,12,18 * * *"XX'u
call-cluster	"0 7,19 * * *"	LIBDIR/VCcall 
		-S _call/hayes.cs # call script
		-D loginstr=sarad
		-D passwdstr=XXXX
		-D phoneno=3234321
		-D modemdev="/dev/cua"
		-D linespeed=2400
		cluster
.ft
.DE
.P
This entry could have been placed on one long line but we have 
spread it over several lines for clarity.
Continuation lines of an entry in
.I initfile
are indicated by leading white space.
.P
The parameters to the call program will be converted to arguments
of the form \fB\-D\fP\fIname=value\fP for use in the 
.I netcall
call script language.
Call script programs are provided in the \f(CW_call\fP directory for several 
popular modems and other connection types.
For a new connection type you can easily write your own call script
program (see
.I  netcall (8)).
.P
Periodic connections involve less overheads than
.I auto-call ,
and allow the scheduling of connections
to take advantage of cheaper billing periods.
.P
In most cases a new connection can be made to the network by adding one line to
\f(CWSPOOLDIR/LIBDIR/initfile\fP.
.H 1 STATISTICS
If you don't want to gather statistics\*F
.FS
Or ``accounting information''.
.FE
at all,
then you should remove the file called
\f(CWSPOOLDIR/STATSDIR/Accumulated\fP
created by the
.I make
target
.I special .
Otherwise, the network will log at least one statistics record in
.I Accumulated
for every message processed.
.P
The statistics are collected as lines containing
`\fB:\fP' separated \s-1ASCII\s0 fields.
The fields are defined in \f(CWInclude/stats.h\fP.\*F
.FS
Should be
.I netstats (5).
.FE
There are no programs as yet to summarize this information
and the file can grow alarmingly quickly on busy systems,
so my advice is to turn off statistics accumulation for the time being.
.H 1 "INTERFACES TO MAIL"
The distribution contains a mail program capable of using the
network to send mail to remote sites,
(see
.I netmail (1)),
and you can arrange for users to access this program
when they wish to send network mail.
Try modifying and installing the shell program \f(CWAdmin/netmail.sh\fP,
which invokes the appropriate mail program depending on its arguments.
.P
However,
if you are running
a 4\s-1BSD\s0 system,
or \s-1SYSTEM V.4\s0,
you already have a version of mail that understands
what to do with network addresses.
You should arrange to cause the mail insertion program to use the
.I "\*(sN"
program
.I netfile (1)
to deliver the message.
We use the sequence:
.P
.DS 1
.ft CW
netfile -Amailer -oP3 -NMail -Q\fIperson\fP\fB@\fP\fIdestination\fP
.ft
.DE
.P
to deliver a mail message read from
.I stdin .
You can, of course, specify many
.I persons
and
.I destinations
at one time to take advantage of
.I "\*(sN" 's
multi-cast addressing capabilities.
Note that the flags \fB-oP3\fP specify that the message should be queued at a fixed,
time-ordered priority,
so that queued mail messages will be delivered in order.
.P
If you don't have one of these systems,
then your best bet is to negotiate with us for a separate mail distribution.
.P
If your mail insertion program has security features to prevent impersonation,
and can arrange to run as a network privileged user (either the
.I super-user ,
or
.I NETUSER )
before invoking
.I netfile ,
then you should edit the
.I handlers
file \f(CWHandlers/handlers\fP (see
.I nethandlers (5))
to turn on the 
.I restriction
flag for the mail handler.
.H 1 "REMOTE PRINTING"
Assuming that you wish to set up a remote printing facility,
the following example sets up the parameters for printing to be allowed
between three hosts ``site1'', ``site2'' and ``site3'',
all running a version of
.I lpr (1)
with some added features.
Let us also assume that the network is being configured at ``site1''
which has a printer, and that ``site2''
has two printers.
.P
The spooler program is called
.I lpr .
It takes some unusual arguments:-
\fB\-h\fP\fIhost\fP to indicate that the job comes from a remote site called
.I host ,
\fB\-u\fP\fIperson\fP to indicate that the job comes from a remote user called
.I person ,
\fB\-n\fP\fIname\fP to indicate the name of the print job,
and \fB\-p\fP\fIprinter\fP to indicate an alternative printer to print the job on.
The handler will pipe the contents of a
.I print
message into the program \s-1PRINTER\s0,
so we should set the Makefile parameter \s-1PRINTER\s0 to be
.I /bin/lpr ,
and \s-1PRINTERARGS\s0 should be set to show the true name
and origin of the job, ie:
.P
.DS 1
.ft CW
PRINTERARGS = \e"-h&O\e",\e"-u&F\e",\e"-n&D\e"
.ft
.DE
.P
Now we should define \s-1PRINTORIGINS\s0 to restrict print jobs to
our own sites, so we set
.P
.DS 1
.ft CW
PRINTORIGINS = /usr/lib/printorigins
.ft
.DE
.P
and in this file we put two lines naming the favoured sites:
.P
.DS 1
.ft CW
site2
site3
.ft
.DE
.P
We also need to enable our users to send
print jobs to the two printers at
.I site2
by creating a file \f(CW/usr/lib/printsites\fP
in which we put just the name ``site2''.
We now use a general feature of the
.I handlers
file (see
.I nethandlers (5))
that allows us to specify an addressing restriction for any message type.
Edit \f(CWHandlers/handlers\fP so that the
.I address
field for the
.I printer
handler contains \f(CW/usr/lib/printsites\fP.
.P
That's all for the Makefile parameters,
but we also need to make it easy
for our users to send their print jobs to ``site2''
and be able to choose what printer it goes on.
This can be done by writing a shell script called, say
\f(CW/usr/bin/netprint\fP.
It could contain the following:-
.P
.DS 1
.ft CW
.ta +.25i +.25i
#!/bin/sh
printer=1
case $1
in
	-1)	printer=1 ; shift ;;
	-2)	printer=2 ; shift ;;
esac
netfile -A printer -D site2 -E "-p$printer" $*
.ft
.DE
The shell program has an optional first argument, either -1, or -2,
which selects which of the printers is to be used. The default is -1.
This information is passed as an argument for the remote spooler
inside the message
.I environment
using the
.I netfile
argument \fB\-E\fP.
.H 1 "INTERFACING TO NETNEWS"
In order to receive news, all that you normally need to do is define
\s-1NEWSEDITOR\s0 when you configure \*(sN, and install
everything normally, and then get your news neighbour(s)
to start sending news to you.
.P
If, however, your news feed wishes to send you batched,
or compressed, news, or some combination, then you will
need to create the \s-1NEWSCMDS\s0 file.
This must have been defined when you configured the
network software as well.
It will usually have been defined to be SPOOLDIR/LIBDIR/newscmds.
Your news feed should be able to tell you what you
need to put in this file.\*F
.FS
If they don't know, then they should probably restrict
themselves to sending simple one-at-a-time articles!
.FE
.P
You are also going to want to send news back to your
news feed, so that local users can post articles.
Since we are assuming that you have read the
netnews documentation, you will know how to put
the following command in your subscription file,
for the node that you are going to send news to.
.P
.DS 1
.ft CW
SPOOLDIR/LIBDIR/sendmsg -dorP9 -A reporter -D <node> -Z 600000
.ft
.DE
.P
That arranges for news articles (which are given
to the command on standard input) to be sent to
the \fIreporter\fP handler at <\fInode\fP>.
The \fBd\fP option prevents \fIsendmsg\fP from
triggering the \fIauto-call\fP mechanism when the news is queued.
The \fBo\fP option has the effect of keeping news articles
ordered, which is generally a good idea, especially
if you are acting as a news feed, and passing lots
of news from one site to another.
The \fBr\fP option prevents news from being returned
to your host if (for some reason) it is not acceptable
at your neighbour node.
In general, this is a good idea, news is seldom
important enough to worry about the small chance
of its being rejected.
However, when you are just starting, you might want
to omit this option.
The \fBP9\fP option causes \fIsendmsg\fP to treat
news as the lowest possible priority traffic.
The \fBZ\fP option specifies that if the news hasn't been delivered
within about one week, then it should be removed.
.P
Note, there is no requirement that the <\fInode\fP>
be a node that is directly connected to your host.
Anyone that you can address is a potential target.
However, sending news articles to sites without their
consent is not a ``nice'' thing to do!
.P
You can use \fIsendmsg\fP's \fB\-E\fP parameter
to pass exotic remote commands to the news system,
should you have need to do that.\*F
.FS
This can be used to send batched and/or
compressed news articles.
The command that you send must be in the
\s-1NEWSCMDS\s0 file at your news neighbour.
.FE
By the time that need arises you will be an expert at
handling \*(sN and the netnews systems,
and will not need explanations from here!
.H 1 "ACTING AS A FILESERVER NODE"
If you desire to, you can make your host available to
others to access files that you might happen to have
and which are generally available to all.
.P
If you are going to do this, then you should
have defined \s-1PUBLICFILES\s0
to be the name of a file or directory that
will give access to the files you will be making
available.
If \s-1PUBLICFILES\s0 is the name of a directory,
then you should make it now.
It can probably be owned by the \s-1SERVERUSER\s0
that you defined to act on behalf of remote users.
If you have any files that you wish to make immediately
available, you can copy, or link, them into this
directory.
Make sure that they are readable by \s-1SERVERUSER\s0!
It's fine to make subdirectories if there is a lot
of related material that should be kept together.
.P
If \s-1PUBLICFILES\s0 is the name of a file, then
you need to create that file.
A minimal case (and probably the best starting
point) would be
.P
.DS 1
.ft CW
.ta \w'friend@neighbourMM'u
*	SPOOLDIR/_public
.ft
.DE
.P
This allows anyone to access files in SPOOLDIR/_public
and is equivalent to simply defining \s-1PUBLICFILES\s0 to
be this directory name.
However, it does leave room for expansion, should you
wish to at some later time.
.P
.I Fileserver
reads this file, forwards, looking for a match of user and address
with those of the user who requested files.
The first line that matches (ie: the first line for
which either the \*(Us part is the name of the remote
user, or is missing or \*(St, and the \*(Ho part
either matches the name of the remote host, or is \*(St)
ends the search.
The
.I full_path_name
on that line is used as the name of the directory containing
files that this particular user may access.
If no line in
.I \s-1PUBLICFILES\s0
matches, or if the line that does match does not
contain a
.I full_path_name,
then the remote user is denied access.
.P
In any case, users are permitted access to files relative
to the final directory only, and may not use a path that
commences with \fB/\fP, or that contains `\fB.\^.\fP' anywhere within it.
.P
Another example might be
.P
.DS 1
.ft CW
.ta \w'friend@neighbourMM'u
network@*	SPOOLDIR
friend@neighbour	/
enemy@*
*	SPOOLDIR/_public
.ft
.DE
.P
This allows requests from the user \fInetwork\fP
at any site to access publicly readable files in your spool directory
(such as \fI_stats/Accumulated\fP
\(em but not network messages,
which are always mode 0600 owned by \s-1NETUSERNAME\s0).
\fIfriend@neighbour\fP is allowed to access any files
on your system, provided that \s-1SERVERUSER\s0 has
read permissions on the requested files.
\fIEnemy\fP is not permitted any access at all, whatever
host he happens to be using, as there is no path specified
for him.
All others can access files in the public directory only.
.P
Once this file is created, you should make sure that any
directories it names exist, and have suitable permissions.
.P
You should also make sure that the user \s-1SERVERUSER\s0
exists in your password file, and that \s-1SERVERGROUP\s0
is in your group file.\*F
.FS
Or password file, if you are running an \s-1AUSAS\s0 system.
.FE
.P
If you wish, the
.I fileserver
handler will keep statistics of all files fetched from your site.
You should define the \s-1FILESERVERLOG\s0 parameter,
and make sure the file exists and is writable by \s-1SERVERUSER\s0,
as otherwise the handler will quietly avoid producing any statistics.
No program exists to interpret the statistics as yet.
.H 1 "ADDING SUN III NODES"
If you have been running with the previous version of the software,
(and you have turned on the \s-1SUN3\s0 gateway option),
then you probably want to add in all the sites that it already knows.
There is a shell script in the directory
\f(CWAdmin\fP
called
\f(CWAddSun3Host.sh\fP
which will output a set of commands suitable for reading into
.I netstate .
You may need to edit it for the local name of your Sun3 state program.
Use it as follows:
.P
.DS 1
.ft CW
sh AddSun3Host.sh | netstate -crs 2>/dev/null
.ft
.DE
.P
or use an intermediate file instead of a pipe,
since you might need to repeat the command.
.P
After the network is running,
\s-1SUN3\s0 state messages will be incorporated automatically
into the new routing tables by the gateway programs.
.H 1 "STARTING THE NETWORK"
Now you can run
.I netstate (8)
to test out your command file (which it reads on seeing the flag \fBC\fP),
let's use the \fBw\fP flag to make it complain about your typos.
We also want to update the statefile and routing tables (\fBrs\fP).
Type:-
.P
.DS 1
.ft CW
netstate -rswC
.ft
.DE
.P
You will probably get warnings from
.I netstate
when it encounters the names of nodes that it doesn't yet know about.
These can safely be ignored, they will cease when the network starts
communicating.
If there were any other complaints, fix up the commands, and try again.
You can use
.I netmap (1)
to see the results of your efforts.
.P
Finally, try writing some call programs and making a few connections.
When a new link is added into the statefile,
.I netstate
will invoke the program
.I netrequest (1)
to broadcast the new topology to all the sites in your
.I "visible region" .
.H 1 MAINTENANCE
You also need to automate the process of getting
.I netinit
started each time your system boots. Just put a line
.P
.DS 1
.ft CW
SPOOLDIR/LIBDIR/start
.ft
.DE
in the usual place,
which would be near where the line printer and other daemons are started in the
.I /etc/rc
file on most systems.
.I Start
can be copied from the file \f(CWAdmin/start.sh\fP,
and will contain all the network initialization commands,
including an invocation of
.I netinit .
You may need to edit it for local directory changes.
.P
Once a night you should run a command from
.I initfile
to clean out temporary files and directories,
and remove uncollected and timed-out file messages:-
.P
.DS 1
.ft CW
.ta +\w'call-munnariXX'u +\w'"0 0,12,18 * * *"XX'u
purge	"0 5 * * *" LIBDIR/purge -ds
.ft
.DE
.P
However,
look at the administration shell scripts ending in \f(CW*ly.sh\fP in the directory
.I Admin
for general guidance.
.P
After system crashes, you might find that the
.I statefile
and/or
.I routefile
got corrupted (by the crash happening in the middle of an file rename).\*F
.FS
This is an extremely unlikely occurrence,
but it always pays to play safe.
.FE
This will cause any programs using the routing tables to fail,
or the state message handler to get an error from
.I netstate .
Repair the statefile by running the command
.P
.DS 1
.ft CW
netstate -ersxC
.ft
.DE
.P
This could also go into your
.I start
file before
.I netinit
is started.
If the statefile gets completely clobbered,
you can request an update from other nodes on the network by running
the program
.I netrequest (1).
(Alternatively, keep a nightly backup of the statefile!)
.P
You should also add a command to ``start'' to remove any lock files
that might have survived a system crash:-
.P
.DS 1
.ft CW
SPOOLDIR/LIBDIR/purge -adl
.ft
.DE
.H 1 UPDATES
From time-to-time,
there may be distributions of updates for your source.
These will arrive over the network as a file called
Sun4.up.\fItype\fP,
where
.I type
is either ``tar'' or ``cpio''
depending on your original choice.
.P
After extracting the new source files,
copy any new
.I Makefile.dist
to
.I Makefile ,
and type \f(CWrun\fP.
When re-compilation has succeeded,
stop any network daemons and do
\f(CWrun install\fP,
followed by
\f(CWnetstate -ersC\fP.
Then restart the network.
.SK
.H 1 SUMMARY
Here is a brief summary of the steps you should have taken,
just in case you missed something.
.AL 1
.LI
Read everything that comes with the distribution, carefully.
.LI
Choose a node name, and the domains it will belong to.
.LI
Extract the tape (or whatever).
.LI
Choose a ``run'' file from the selection in the Makefiles directory.
.LI
Edit ``run'' to set system dependent definitions.
(Section 3.)
.LI
\fBrun\fP. (Section 5.)
.DL
.LI
If anything goes wrong, fix it and \fBrun\fP again.
.LI
Repeat.
.LE
.LI
\fBrun\ directories\ special\ install\fP
.LI
Edit \fISPOOLDIR/STATEDIR/commandsfile\fP.
(Section 7.)
.LI
\fBnetstate -rswC\fP
(Section 17.)
.LI
Repeat steps 8 and 9 until it's right.
.LI
Start the network.
.LE
.TC
.CS
