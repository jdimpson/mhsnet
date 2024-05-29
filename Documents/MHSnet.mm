.\" pic % | tbl | troff -rL19.5c -rW11c -rO3c -mm | lw -r
.\" OPTIONS mm
.\" SCCSID @(#)MHSnet.mm	1.35 05/12/16
.nr S3 0 \" 1 for ACSnet
.SA 1
.MT 4 l
.nr C 3\" DRAFT (to be effective MUST be set on command line as -rC3)
.ds HF 3 3 3 3 3 3 3\" all heads bold
.nr Hu 4\" unnumbered heads at level 4
.nr Hb 7\" no run in heads
.nr Hs 3\" half line space after A level head
.nr N 2\" no header on first page
.nr Hy 0
.ds sM \s-1SUN III\s0
.ds sN \s-1MHS\s0net
.ds sD \s-1\f(NBSPOOLDIR\fP\s0
.de HX
.if \\$1=1 \{.OH "'''\\s10\\fH\\$3\\s0\\fP'"
.	EH "'\\s10\\fH\\\\*(hD\\s0\\fP'''"\}
.ps 12
..
.de HY
.ft H
..
.de HZ
.\"if \\$1=1 \{.OH "'''\\$3'"
.\"	EH "'\\\\*(hD'''"\}
.ps 10
.ft R
..
.de Hd\" new section macro
.nr Ej 0\" turn of page ejects for first level 1 heading
.nr H1 0\" reset heading number
.nr P 0\" reset page number
.ds hD \\$1
.OH "''''"\" no page headings on first page of section
.EH "''''"
.bp 1
\s+4\f(HB\\*(hD\fP\s-4
.sp 2c
\l'\n(.lu'
.sp
.H 1 "\\$2"
.nr Ej 1\" new page for subsequent level 1 headings
..
.de Cn\"command name macro
\f(CW\\$1\fP\\$2
..
.PH "''''"
.OF "'\\\\*(sN'\\\\s-2Copyright \\\\(co GPL\\\\s+2'\\\\nP'"
.EF "'\\\\nP'\\\\s-2Copyright \\\\(co GPL\\\\s+2'\\\\*(sN'"
.bp
.Hd "\*(sN Users Guide" "Introduction"
\*(sN is a
.I "message handling system"
that allows users to send
messages of any size and containing any type of data
to other users through a network of Unix based systems.
The messages are transported through the network in a store-and-forward manner
being moved from machine to machine until the destination system is
reached.
.P
\*(sN allows you to send electronic mail, documents, data files and
programs to users on computers elsewhere in the world.
.H 1 "How it works"
\*(sN consists of three main parts or
.I layers .
The top layer is the user interface layer with applications such as
electronic mail and file transfer programs.
The normal user interacts with this layer of programs and a set of
management programs that display network information.
.P
The middle layer is the routing layer.
This part of the system takes a message and decides where to send it
based on the shape of the network and what the best route is to the
destination.
.P
The lowest layer of the system is the machine to machine transfer layer.
This layer uses a sophisticated protocol to transfer each message between
machines, recovering from errors when necessary.
The connection between machines can be made using telephone lines,
X25 networks such as AUSTPAC, and Ethernet (using the TCP/IP protocol).
.P
\*(sN provides a message delivery service that allows an
application program to send a message to another program on some remote
machine without having to worry about the route it will take through the
network or the particular connections between machines.
The electronic mail and file transfer services provided with MHSnet make
use of this message delivery service.
.P
The user of the electronic mail program composes a message and specifies
that it go to one or more remote users. The mail program accepts the
message and passes it to the MHSnet software which determines the route
through the network and what machine to machine links to use.
At the destination the message is passed to the electronic mail delivery
program which places it in the mail box of the destination user.
.P
The file transfer system also uses the message delivery service.
A user specifies the file(s) to send and the destination user(s)
using the command ``netfile user@address filename''.
Netfile passes the file to \*(sN for delivery.
When the file is delivered to the destination machine it is placed in a
holding directory and electronic mail is used to notify the recipients
that a file has arrived for them. The recipient can then retrieve
the file from the holding area.
.P
Apart from electronic mail and file transfer, \*(sN provides
remote printing, a simple directory service and a file repository service.
Other message oriented services can be added and applications
that require a message delivery service can easily make use of MHSnet.
.H 1 "Network Addresses"
An \*(sN user specifies the destination for messages using a
.I "network address" .
Network addresses are used by the software to select the destination
machine and user.
.P
Network addresses consist of several parts: the
.I "login name"
of the destination user and several
.I "domain names"
that specify the destination machine.
The domain names are a little like parts of a postal address but instead
of specifying physical locations like a street name or city they specify
the name of the machine and the organisation and department it belongs to.
The only ``geographical'' part of the address is the country name.
.P
In an \*(sN network addresses have the following parts:
.TS
tab(:) ;
l l .
COUNTRY:The country code (Australia is ``AU'')
PRMD:The ``Private Management Domain Name'' (network name)
ORG:A code name for your organisation
DEPT:A code name for your department
NODE:The node name of your machine
.TE
Addresses are written as a sequence of
domain names with dots between them as in:
.DS
.ft CW
.ce
altair.mkt.XYZ.MHS.au
.ft
.DE
Where the node name of the machine is ``altair'', the department
is ``mkt'', the organisation code is ``XYZ'', the network name
is ``MHS'' and the country code is ``au'' for Australia.
Note that case is immaterial.
.P
Electronic mail messages have an address that includes the
login name of the recipient.
To send mail to a user with login name ``bill'' at our example machine
we would use the address:
.DS
.ft CW
.ce
bill@altair.mkt.XYZ.MHS.au
.ft
.DE
.H 1 "Sending Electronic Mail"
To send a message to another user you use your electronic mail program.
There are many different electronic mail packages available for Unix
ranging from a simple one that comes with all Unix
systems to mouse and window based programs.
You probably have a sophisticated mail program installed on your system,
possibly even a complete office automation package that includes an
electronic mail package. In that case  you should refer to the
documentation of that package for instructions on how to use it.
.P
All the electronic mail packages have one thing in common: you must
specify an address for each of the users that you wish to send the
message to.
If the destination user is on another machine this address will be a
network address in the form we described earlier.
When the electronic mail program sees a network address rather than an
address of a user on your local machine, it will pass the message to
\*(sN for delivery.
.H 1 "Sending Files"
Electronic mail is probably the most common form of ``message'' people
send to each other on a computer network.
Mail is useful for general office correspondence but is not appropriate
for sending a data file or a program.
This is because electronic mail
systems only handle text and not arbitrary binary information.
With \*(sN you can send any sort of data file containing any type of
information using the
.Cn netfile
command.
.P
.Cn Netfile
is very easy to use: you simply nominate the file you wish to send and
the address of the destination user.
Here is an example:
.DS I
.ft CW
netfile kim@altair.mkt.XYZ.MHS.au datafile
.ft
.DE
The file called
.Cn datafile
will be sent to the user ``kim'' on machine ``altair''.
.Cn Netfile
allows you to send many files in a single command to several users.
For example, the command:
.DS I
.ft CW
netfile bob,judy@altair.mkt.XYZ.MHS.au *.data
.ft
.DE
will send all files in the current directory whose names end in
``.data'' to the users ``bob'' and ``judy'' on machine ``altair.mkt.XYZ.MHS.au''.
.P
When a file arrives for a user it is placed in a special holding or
.I spool
directory and a short electronic mail item is sent to the user telling them
that a file has arrived.
The message gives the name of the file, who it came from, how big it is
and other information.
It also says that the command
.Cn netget
should be used within a certain time (usually 7 days) to retrieve the
file from the holding directory.
.P
The
.Cn netget
command will prompt the user for each file held in the holding directory
asking if it should be retrieved or some other action performed.
If a file is to be retrieved and placed into the current directory the
user answers ``yes'' to the prompt, otherwise one of the other
actions such as ``rename'', ``delete'', ``cd'' etc can be given.
For a complete list of actions you should consult the
.Cn netget(1)
Unix manual entry.
.P
Here is an example of using
.Cn netget
.DS I
.S -2
.ft CW
$ netget
1 file spooled.
From                         Size    Modified    File
bob@server.personis.os.au    2114  Nov 11  1987  datafile
 (already exists) ? y
$
.ft
.S +2
.DE
.bp
.Hd "\*(sN Installation Guide" "Introduction"
This section of the manual describes the installation procedure for \*(sN.
You should read it carefully before beginning the installation.
.P
In summary, the steps necessary to install \*(sN are:
.AL i
.LI
read this section of the manual
.LI
complete the configuration form for your site
.LI
collect the necessary information (phone numbers etc) for machines you
wish to connect to
.LI
load the software into your system
.LI
run the install program and follow its instructions
.LE
.H 1 "System requirements"
.HU "Disk space"
\*(sN requires at least 3 Megabytes of disk storage for the programs and
data files that make up the system.
You will also need disk space for
messages to and from your users as well as message in transit through
your machine.
On a busy system, used as a ``backbone'' machine
for a large network, as much as 40 Megabytes may be required for message
storage.
On a small system, the space required may be as low as 5 megabytes.
.HU "Connections"
To make a connection to other machines on the network you will need some
form of communications interface.
The most common connection type is an asynchronous serial RS232 port.
Almost all Unix systems have such ports for connection of terminals,
modems or direct lines to other computers.
.P
\*(sN can very easily make use of a serial port for connection to
another machine.
This could be done using a direct fixed line between the machines
such as a wire between nearby machines or a leased line from
Telecom.
Alternatively, the serial port could be connected to a dialup modem
and \*(sN can be instructed to call the other machine,
or be called to transfer messages.
.P
Other connection types that can be used are X25 and TCP/IP.
If your system has the hardware and software for these protocols, \*(sN
can make use of it to connect to other similarly equipped machines.
.HU "CPU time"
\*(sN consumes only a modest amount of processor time depending on the
amount of message traffic that is processed.
.H 1 "Directories"
The programs and data used in \*(sN are installed in your machines file
system in the following directories:
.VL "\w'/usr/spool/MHSnetXX'u"
.LI /usr/spool/MHSnet
Message files, \*(sN data files and system programs are stored in this
area.
The name of this directory can be changed at installation time,
and will be referred to as \*(sD in following text.
.LI /usr/bin
User programs such as ``netfile'', the file sending program are placed
in this directory.
.LI /usr/lib
The \*(sN parameter file, ``MHSnetparams'', is placed in this directory.
.LI /usr/man
The \*(sN manual entries for sections 1 through 8
of volume 1 of the standard \s-1UNIX\s0 manual.
.LE
.H 1 "Network Addresses"
Each machine in the network has a
.I network
.I address .
This address is used by \*(sN and your users to identify your machine and
so must be unique.
.P
An address consists of a number of parts, much the same as a postal
address consists of a street number, suburb, postcode etc.
In \*(sN the parts are:
.TS
tab(:) ;
l l .
COUNTRY:The country code (Australia is ``AU'')
PRMD:The ``Private Management Domain'' (network name)
ORG:A code name for your organisation
DEPT:A code name for your department
NODE:The node name of your machine
.TE
For example, a machine in the marketing group of XYZ Pty Ltd
might have the following address:
.TS
tab(:) ;
l .
COUNTRY=AU
PRMD=MHS
ORG=XYZ
DEPT=mkt
NODE=altair
.TE
AU is the
International Standard 2 letter country code for Australia.
.P
The name chosen by XYZ Pty Ltd for the network is MHS and this is
recommended if you don't have a large network.
The PRMD of ACSnet, the Australian academic and research network, is ``oz''.
.P
The organisation and department codes should be a meaningful word
related to the organisation or department names.
.P
You may break up your organisation into departments and sub-departments
or organisational units.
If this is the case you can use the address component types
``\s-1OU1\s0'', ``\s-1OU2\s0'', ... etc instead of ``\s-1DEPT\s0''.
.P
The node name must be unique within your department.
It is suggested that you shouldn't base the node name on the type of
machine as this may change.
You should also avoid node names containing equivocal characters such as
``1'' (one) or ``l'' (el).
People will use this name in conversation to refer to the machine so
be creative!
.P
The codes used in each part of an address
must consist of letters from the set
.B [\-_A-Za-z0-9] .
.P
When an address is used in \*(sN it can be written in two ways.
In the first, each part of the address includes its type and is
separated from the next part by a dot.
So, our example machine from XYZ Pty Ltd would have the address:
.DS
.ft CW
NODE=altair.DEPT=mkt.ORG=XYZ.PRMD=MHS.COUNTRY=au
.ft
.DE
Note that the case of the characters in the address is ignored.
By convention, the types are written in upper case.
.P
The second, and more common way that addresses are written is to omit
the type information.
In this form the address of our example machine is:
.DS
.ft CW
.ce
altair.mkt.XYZ.MHS.au
.ft
.DE
.P
Messages such as electronic mail that are sent to users include the
login name of the recipient in the address.
The login name is followed by an at sign (@) and then the machine address.
To send mail to a user with login name ``bill'' at our example machine
we would use the address:
.DS
.ft CW
.ce
bill@altair.mkt.XYZ.MHS.au
.ft
.DE
.HU Regions
Each part of the address is also known as a
.I domain
and
concatenated domain names make up names of
.I regions
in the network.
The region consisting of all the domains except \s-1NODE\s0 is commonly referred to as your
.I site
address.
.P
One of the reasons for regions is to reduce the
number of sites that must be remembered in the network routing database.
A lot less information is needed to remember the route to a region,
than to remember the routes to all the members of that region.
This is brought about if all those nodes which are essentially local
workstations are relegated to site specific regions.
Knowledge of the local nodes will not appear in the databases of
nodes at other sites,
yet they may still be addressed explicitly by prepending the node name
to the site address (which \fBis\fP known.)
There is another advantage \(em you may choose node names that aren't
unique across the whole network,
yet which become unique when the site address is included.
Nodes are just special cases of typed domain names,
so domain names themselves also conform to this rule:
domain names become unique in a wider context
by appending the name of an encompassing region.
.P
Domain names are typed,
and so different types may share the same name.
For example, \f(CWORG=XYZ\fP and \f(CWDEPT=XYZ\fP are two different domains,
but beware that if two such identical names occur,
then you may be forced to use the type qualifier to distinguish between them.
.P
The name of a domain (or node) must be unique within its region.
.H 1 "Configuration form"
In order to have all the necessary information available during installation
we have included a configuration form in this manual for you to
complete.
The installation program will ask for this information during the
installation process.
.P
At the end of this section we have included a blank form as well as an
example configuration form as a guide.
.H 1 "Connections to Other Machines"
\*(sN can connect to other machines using a variety of methods.
In this section we describe the different connection methods and the
information you will need for that connection type.
.P
In each case you will need the full network address of the machine you
are connecting to.
.H 2 "Direct Connections"
This is the simplest form of connection.
It normally uses a simple RS232 asynchronous communications interface
with a cable going directly to a similar interface on the remote
machine.
The remote machine may be located in the same room as your system or,
using a leased line, may be anywhere else in the world.
.P
Apart from the full network address of the remote machine, the only
other information required to configure the link is the device name of
the interface and the line speed.
.H 2 "Dialup Lines"
Dialup lines are similar to direct connections in that they use RS232
asynchronous interfaces but they employ modems that are connected to the
telephone system.
One machine can call another by sending dial commands to the modem
specifying the phone number.
Once the connection is made, the calling machine can ``login'' to the
remote machine using a user name and password and the network programs
then begin operation.
.P
For a dial
.I in
connection only the network address of the remote machine is needed.
However, you will also need to make an account for the remote machine
to login to.
.P
.ne 10
For a dial
.I out
connection you will require:
.AL 1
.LI
device name of the interface
.LI
line speed
.LI
the phone number of the remote modem
.LI
the modem type
.LI
tone or pulse dialing
.LI
login name and password at remote machine
.LE
.H 2 "Ethernet (TCP/IP)"
For incoming or outgoing TCP/IP calls you will have to configure your
TCP/IP information files with information about the remote host.
You should consult your TCP/IP manual for information on how to do this.
\*(sN requires the ``IP'' address of the remote host in addition to its
\*(sN address.
.H 2 "X25"
For X25 connections you will have to configure your
X25 information files with information about the remote host.
You should consult your X25 manual for information on how to do this.
.P
For outgoing calls \*(sN requires the X25 address of the remote machine
in addition to its \*(sN address.
.if \n(S3 \{\
.H 2 "SUN III"
It is possible to make connections to sites running the old version of the software,
known as \*(sM
(as used by many sites on the Australian Computer Science Network, a.k.a. ``ACSnet'').
Answer ``yes'' when the configuration program asks you:
``Is this a connection to an ACSnet site with SUN III?''.
.P
Special transport daemons exist for each of the four different protocols
in use by the \*(sM software,
and call scripts are available to establish connections using them.
Messages passing via these connections must be transformed between the old and new message formats,
and this is achieved by specifying the
.I filter
program ``filter43'' for a link to any \*(sM site.
Note that the \*(sM software cannot negotiate connection parameters,
and so the call accepting site must be pre-configured with any non-default connection parameters.
Read the section on \*(sM at the end of the \f(HBManagement Guide\fP (section 12),
and also read the
.I netcall (8),
.I netdaemon (8)
and
.I netshell (8)
manuals for further details on making \*(sM calls.\}
.H 1 "Installation"
To begin installation of \*(sN:
.AL 1
.LI
Login as "root".
.LI
Load MHSnet diskette 1 into your floppy disk drive.
.LI
Type the following command:
.DS
.ft CW
cd /tmp
tar xf /dev/rfd/00
.ft
.DE
.LI
When the diskette has been read type the command:
.DS
.ft CW
\&./installmhs
.ft
.DE
The install program will ask you to load further floppies.
It will install the programs and data files and then ask a series of questions.
You will need the information from your configuration form
to answer most of the questions.
.LI
When the installation program finishes you may need to add entries to
.Cn etc/passwd
for the accounts to be used by
the network programs.
These accounts may already be installed in your system,
but the install program will have told you if you needed to create any.
These accounts should have
a password of
.Cn * ,
and
.Cn HOME
directory
.Cn /tmp .
For example, the following line could be used:
.DS
.S -2
.ft CW
daemon:*:UID:GID:MHSnet account:/tmp:
fileserver:*:UID:GID:MHSnet file service:/tmp:
.ft
.S
.DE
where UID is an unused user id number and GID is an unused group id
number.
You may also need to add new groups to the
.Cn /etc/group
file.
For example the following entries could be used:
.DS
.S -2
.ft CW
daemon::DGID:daemon
fileserver::GID:fileserver
.ft
.S
.DE
where GID is the same as the group id used for fileserver in /etc/passwd
and DGID is an unused group id number.
.LE
.H 1 "Configuring the network"
At the conclusion of the installation procedure you will be asked if you
wish to configure the network at that time.
If you answer ``yes'' the \f(CWconfigmhs\fP program will be started.
.P
You can run the configuration program at other times by typing the
commands:
.DS
.ft CW
cd /usr/spool/MHSnet/_config
\&./configmhs
.ft
.DE
.P
.Cn Configmhs
is a simple menu oriented program that helps you set up
links to other machines and modify various configuration parameters.
It allows you to enter or edit site details and to
display or edit details of connections to other machines.
.P
After reading the manual sections covering the
.I commandsfile ,
.I initfile
and
.I netcall
you may wish to edit the \*(sN files directly without using
.Cn configmhs ,
however you cannot then go back to using
.Cn configmhs .
.P
Network managers should include the path name of the network
managment binaries in their shell PATH variable, after which
.Cn configmhs
may be invoked as
.Cn netconfig :
.DS
.ft CW
PATH=$PATH:/usr/spool/MHSnet/_bin
export PATH
.ft
.DE
.H 1 "Starting the network"
The configuration process has created a number of network configuration
files.
The most important of these are
.I commandsfile
and
.I initfile .
They contain information from your configuration form.
For a detailed description of commandsfile and initfile
see the System Reference Manual and the entries for ``netstate''
and ``netinit'' in section 8 of the manual.
.P
You may get warnings from
.I netstate
when it encounters the names of nodes that it doesn't yet know about.
These can safely be ignored, they will cease when the network starts
communicating.
.P
You can start the network now by typing:
.DS 1
.ft CW
/usr/spool/MHSnet/_lib/start
.ft
.DE
.P
This shell script will perform some cleanup operations and then start
the
.Cn netinit
program.
.Cn Netinit
reads its parameter file (initfile) and runs network processes as needed.
These include the routing program, administrative programs and call
programs that establish network connections with other machines.
See the section of the System Reference Manual
entitled ``Network Process Management'' and the
manual entry ``netinit(8)'' for more details.
.HU "Starting the network after reboot"
You need to automate the process of getting
.I netinit
started each time your system boots. Just put the line
.P
.DS 1
.ft CW
/usr/spool/MHSnet/_lib/start
.ft
.DE
in your
.I /etc/rc
file near where the line printer and other daemons are started.
.bp
.Hd "\*(sN Management Guide" "Introduction"
\*(sN is a message handling system that provides message
services such as electronic mail, file transfer, news and directories
across a wide area network of computers running the Unix operating
system.
The system is constructed in three layers:
.BL 6
.LI
message protocol layer
.LI
routing layer
.LI
node to node transport layer
.LE
.P
Message service protocols are provided at the message protocol layer.
The layers below the message protocol layer provide an end to end
message delivery service. A message service such as electronic mail can
request that a mail item be delivered to the mail system on a remote
machine. The routing layer at each machine is then responsible for
passing the message from machine to machine across the network until the
destination is reached and the message is passed to the message protocol
layer for handling. The node to node layer transports messages between
machines.
.P
\*(sN is controlled by a number of command and parameter files:
.VL 0.5c 0.5c
.LI \*(sD\f(CW/_state/commandsfile\fP
your network node description, specification of links to other nodes
.LI \*(sD\f(CW/_lib/initfile\fP
specification and control of network processes for routing,
administration and connections to other nodes
.LI \f(CW/usr/lib/MHSnetparams\fP
specification of \*(sN parameters
.LI "call\ scripts\ in\ \*(sD\f(CW/_call\fP"
programs that specify how connections are established with remote nodes
written in a special purpose language
.LE
.P
The
.Cn commandsfile
specifies information about the node such as its full
address, type hierarchy and mapping information.
It is mandatory to supply some specific site description details,
such as organisation name, and contact phone number.
Links are parameterised with cost and message delay times,
and any addressing restrictions for messages that may be carried on the link.
.P
The
.Cn initfile
specifies network processes that are to be run by
.Cn netinit .
These include the network routing program, administration programs that
clean up network data files at regular intervals and call programs that
establish connections with remote nodes.
.P
A call script specifies how a connection is to be
made on a link. The detailed call establishment procedure is described
in a program written in a new language designed for the purpose.
Call scripts are provided with \*(sN for different connection types, however
system administrators can write their own for new connection types.
.P
The
.Cn commandsfile
and
.Cn initfile
are normally constructed by the
.Cn configmhs
program.
To gain finer control over the network
system administrators can modify these files directly, however this
would not normally be necessary in most networks,
and note that you cannot then re-use the
.Cn configmhs
program (which overwrites these files).
.H 2 "System Structure"
The system is constructed in three layers: a message handling layer, a
routing layer and node to node transport layer. Standing beside this
are management programs that control the execution of the
programs that implement these layers.
.bp
.DS
.ft H
.PS 3.75
		circlerad = 0.1
IFILE:		box "init" "file"
		arrow -> down from bottom of IFILE
NETINIT:	ellipse "netinit"
		move down
NETFILE:	ellipse "netfile"
		move down
NETMSG:		ellipse "netmsg"
		move down
NONE:		ellipse invis "..."
		move down
STATER:		ellipse "stater"
		move to right of IFILE
		move right
CSCRIPT:	box "call" "script"

		arrow -> down from bottom of CSCRIPT
NETCALL:	ellipse "netcall"

		arrow dashed right from right of NETINIT

		move down from bottom of NETCALL
NONE2:		ellipse invis "..."
		move down
		move down
ROUTER:		ellipse "router"
		move down
		move down
NETSTATE:	ellipse "netstate"

		arrow dashed from STATER.e to NETSTATE.w
		arrow dashed from NETINIT.se to NONE2.nw

		move down 0.5 right 0.5 from NETFILE
QA:		circle "\s-2Q\s+2"
		arrow from NETFILE.se to QA.nw
		arrow from NETMSG.ne to QA.sw
		arrow from QA.se to ROUTER.nw
		arrow dashed from ROUTER.w to NONE.ne
		arrow dashed from ROUTER.sw to STATER.ne

		line <- down 0.25 from ROUTER.s
ROUTEFILE:	box "route" "file"
		arrow from NETSTATE.n to ROUTEFILE.s
		move down left  from NETSTATE.s
COMMANDSFILE:	box "commands" "file"
		move right
		move right
EXPORT:		box "export" "state"
		arrow from COMMANDSFILE.n to NETSTATE.sw
		arrow from NETSTATE.s to EXPORT.n
		line <-> right from NETSTATE.e
STATEFILE:	box "state" "file"
		line <-> right from ROUTER.e
QB:		circle "\s-2Q\s+2"
		line <-> right
DAEMON:		ellipse "daemon"
		arrow dashed from NETCALL.se to DAEMON.nw
		arrowwid = 0.1
		arrowhead = 7
		line <-> right 0.25 from DAEMON.e
.PE
.ps 0
.ft P
.DE
In the diagram, boxes indicate files and ovals indicate programs.
Circles with the letter ``Q'' inside indicate message queues.
Solid lines indicate data flow and dashed lines indicate flow
of control.
.P
At the message protocol layer there are programs that send messages
(netfile, netmsg). These programs may be invoked by user agent
programs for services such as electronic mail.
.P
Also at the message protocol layer are the handler programs that receive
messages. These may pass the message to user agent programs such as a
mail delivery system or process the message completely such as the
simple directory service.
.P
At the routing layer the ``router'' program receives messages for
transmission or delivery and decides on the basis of routing tables how
to dispose of the message.
.P
At the node to node transport layer a daemon process transfers the
message to another host.
.P
The management programs that control the system include ``netinit'' for
invoking network programs at particular times, netcall for establishing
connections with another host, a message handler for network state
messages and ``netstate'', a program to maintain routing tables.
.bp
.H 2 "Node to Node Protocol"
The node to node message transport protocol
is a full duplex protocol that
multiplexes transmission of several messages in each direction
simultaneously.
This allows higher priority messages to overtake lower priority.
Nine priority levels are specifiable by message senders.
.P
The protocol has been designed for maximum throughput with minimum
transmission costs,
and to avoid interference with link-level protocols such as X.25, TCP/IP,
or modems with internal buffering protocols.
The protocol allows maximum throughput
even when many small messages are being transmitted.
Input and output are decoupled by using one process for each,
and all incoming messages are handled by a single permanently running routing process.
.H 1 "The Commandsfile"
The file \*(sD\f(CW/_state/commandsfile\fP is used
to control the behaviour of the program
.I netstate (8).
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
You should read the
.I netstate (8)
manual thoroughly.
.P
Here is a sample
.I commandsfile
from the node ``altair''.
It should help you master most of the possibilities.
It is split into fragments, with comments after each one.
Note that fields are separated by white space (\fItabs\fP,
.I newlines
and/or
.I spaces ).
Any white space
.B within
fields must be
.I escaped
with a `\e' (back-slash) and any `\e' must also be escaped (i.e.
written `\e\e').
.br
.ne 10
.H 2 "The local node"
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
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
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Map in some real names:
#
map	Australia	C=au
map	Austria	C=as
map	MHS	P=MHS
map	New\e Zealand	C=nz
map	Switzerland	C=ch
map	XYZ	O=XYZ
map	mkt	D=mkt
.ft
.DE
.S
.P
Below we map some real names for the commonly used typed domains.
In particular, note that our department's name is
``mkt''
(for marketing),
the organisation is
``XYZ''
(for XYZ Pty Ltd),
the private management domain is
``MHS''
(for the generic message handling network),
and the country is
``Australia''.
Note that spaces are legal inside names
when escaped with `\e'.
We will use some of these names in the following commands,
just because they are prettier than the typed names
(which could be used instead.)
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'XYZ'u
#
#	Our address:
#
address	N=altair.mkt.XYZ.MHS.Australia
#
#	visible within MHS:
#
visible	N=altair.mkt.XYZ.MHS.Australia
	MHS.Australia
.ft
.DE
.S
.P
First we use the domain names to define the
.I "region address"
of the node we are configuring.
The node name is
``altair'',
and the remaining domains are selected from those defined above.
The second command defines the region within which
.I altair
will be able to resolve addresses,
and within which it will propagate its routing information.
Note that the full region address must be used,
and that commands may be continued onto successive lines
by preceding each successor with a leading tab.
.P
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
In our XYZ example,
the local domain is called ``\s-1DEPT\s0=mkt''
(for ``Marketing Department'').
We have nodes that are local to the department whose
visible region is therefore:
.DS
.ft CW
DEPT=mkt.ORG=XYZ.PRMD=MHS.COUNTRY=au
.ft
.DE
However,
the main nodes communicate with other departments,
and so their visible region is
``PRMD=MHS.COUNTRY=au''.
In fact, all sites in Australia should have an address ending in
``.COUNTRY=au'', or, to put it more generally,
the domain denoting your country should always be one of the domains
mentioned in the address.
(Note that because the domains are
.I typed
they don't have to appear in any particular order,
although the convention is right to left in order of increasing significance.)
.br
.ne 20
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Site details:
#
organization	XYZ Pty Ltd
postal_address	Marketing Department \e
	XYZ Pty Ltd \e
	201 Pitt St \e
	Sydney \e
	N.S.W. 2000\e
	Australia
system	IBM 3090
location	33 53 25.0 S / 151 11 18.7 E
remarks	Market Forcasting
person	Kim Smith
email_address	kim@mkt.XYZ.MHS.au
telno	+61-2-321-9876
.ft
.DE
.S
.P
The site details are for the information of other sites on the network.
Two of the commands are mandatory,
.I organisation
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
.I "without types" ,
(\fIsite\fP means the \s-1NODE\s0 domain is omitted).
Node names are discouraged in mail addresses since they are more likely
to change as you upgrade or buy new machines.
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
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Map an easier handle:
#
map	A	N=altair
.ft
.DE
.S
.P
This is just an
.I alias
for the node, for the convenience of poor typists.
It is a local name only, but can be used to refer to
the local node in addresses, as in: \f(CWnetmap -v a\fP
(case is not significant.)
.H 2 Links
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Configure the links:
#
add	N=sol.XYZ.MHS.Australia
link	N=altair.mkt.XYZ.MHS.Australia,
	N=sol.XYZ.MHS.Australia
linkname	N=sol.XYZ.MHS.Australia
	sol
.ft
.DE
.S
.P
This is an example of a link into ``altair''.
We first
.I add
the node's address, then make a bi-directional link to it,
(uni-directional links are made with the
.I halflink
command.)
Note that links are specified by a comma separated pair of node addresses.
Then we give the link a short directory name for ease of use.
The default would be a directory name constructed from the reverse ordered
internally typed address in lower case, i.e. ``0=au/2=mhs/3=xyz/9=sol''.
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
add	N=wild.O=UVW.P=com.C=us
link	N=wild.O=UVW.P=com.C=us,
	N=altair.mkt.XYZ.MHS.Australia
delay	N=wild.O=UVW.P=com.C=us,
	N=altair.mkt.XYZ.MHS.Australia
	43200
restrict	N=wild.O=UVW.P=com.C=us,
	N=altair.mkt.XYZ.MHS.Australia
	XYZ.MHS.Australia
map	wild.usa
	N=wild.O=UVW.P=com.C=us
.ft
.DE
.S
.P
The second link example is to a site
that calls us every twelve hours,
so we set the maximum queuing delay for messages to be the same (in seconds.)
We wish to receive only company traffic over this link,
so a
.I restriction
is placed on the link into ``altair''
limiting traffic from ``wild''
to messages addressed to sites within ``XYZ.MHS.Australia''.
(They have a similar restriction on their end limiting traffic from us
to messages addressed to sites within ``O=UVW.P=com.C=us''.)
Finally we map an old address for this site into the real one,
for the convenience of users.
.H 2 "Address Forwarding"
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Forward all messages addressed explicitly
#	to our department, or XYZ:
#
forward	mkt.XYZ.MHS.Australia
	N=cluster.mkt.XYZ.MHS.Australia
forward	XYZ.MHS.Australia
	N=sol.XYZ.MHS.Australia
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
The node ``sol'' is nominated to handle company-wide messages.
.H 1 "Calling Other Systems"
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
transport daemon running), then the
.I auto-call
facility must be enabled.
If you wish to make connections at regular times, then the
.I netinit
facility can be used as described in section 4 of this management guide.
.P
The
.I auto-call
facility is provided by the program
.I netcall (8).
This program in turn provides support for running call
.I programs
written in an appropriate interpreted language
to control the establishment of a virtual circuit to some remote
node over some combination of intervening networks.
The successful completion of the program will result in
two transport daemons communicating with each other
over the virtual circuit.
.I Netcall
expects at least one argument, which is the name of a program to run.
.H 2 "Enabling Auto-call"
The routing program knows how to invoke
.I netcall
with a program named ``call''
if the link is suitably flagged,
and there is indeed such a file name
in the link's queuing directory.
As an alternative to
.I netcall
you can specify a different call program
using the
.I netstate
command
.I caller .
If no ``call'' file exists,
and you haven't specified a different call program,
then the routing program will invoke the program
.I netcontrol (8)
to start any command that matches the (full, untyped) name of the link.
.P
You may create a call program
to make the connection and start the daemon.
There are several examples in the
.I netcall
distribution directory
\*(sD\f(CW/_call\fP.
Put a copy of the program in the link's queue directory
in a file labelled
.I call .
Then add a line to
.I commandsfile
to let the routing program know:
.P
.S -2
.DS 1
.ft CW
.ta +\w'postal_addressXX'u +\w'Marketing DepartmentXX'u
#
#	Flag the auto-call links:
#
flag	N=altair.mkt.XYZ.MHS.Australia,
	N=wild.O=UVW.P=com.C=us
	+call
.ft
.DE
.S
.H 2 "Accepting Calls"
Alternatively, if other nodes are going to call you,
then you need to create a
.I login
name for them to connect to.
The method of doing this is idiosyncratic to your brand of \s-1UNIX\s0,
but on most versions it consists of using the editor
to add an extra line to the file
\fI/etc/passwd\fP.
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
to be invoked must be set to be the path name of an
appropriate installed version of
.I netshell (8).
(Versions of
.I netshell
exist for many types of virtual circuit.)
It is also advisable to set a password
for the account.
Some versions of
.I login (1)
restrict the length of a login name allowed.
If yours does this and the name of one of the nodes dialing in is too long,
then you can add an alias that is shorter,
and
.I netshell
will look it up to find the real node name.
.P
For example, the following line was added to /etc/passwd to allow a
machine at UVW Corp. in the US to connect to us.
.P
.DS
.ft CW
.ps -2
wild::64:1:UVW login:/usr/spool/MHSnet:_lib/ttyshell
.ps +2
.ft
.DE
.P
Accounts of this type should be given a password.
The calling program at the remote machine will have to quote this
password when logging in to create a network connection.
.H 2 "TCP/IP Connections"
\s-1TCP/IP\s0 connections are handled on the caller side
by the shell script \*(sD\f(CW/_call/call.tcp\fP,
and on the callee side, by \*(sD\f(CW/_lib/tcpshell\fP.
.P
A suitable entry should be made in the
.if t \f(CW/etc/servers\fP
.if n `/etc/servers'
file,
eg:
.P
.DS
.ce
.ft CW
mhsnet tcp \*(SR/_lib/tcpshell
.ft
.DE
.P
or in the
.if t \f(CW/etc/inetd.conf\fP
.if n `/etc/inetd.conf'
file,
eg:
.P
.DS
.ce
.ft CW
.if\n(.lu<6i .ps -2
mhsnet stream tcp nowait root \*(SR/_lib/tcpshell tcpshell
.if\n(.lu<6i .ps
.ft
.DE
.P
If your system lacks a version of 
.if t \f(CW/etc/inetd.conf\fP
.if n `/etc/inetd.conf'
(on some systems called
.if t \f(CW/etc/servers\fP),
.if n `/etc/servers'),
you may have to make use of the program
.IR netlisten (8).
.P
The callee is advised to
set a password to guard the \s-1TCP/IP\s0 connection
(or any \*(sN connection)
by using \fInetpasswd\fP to set a site-specific
(or region-specific) password
(see
.I netpasswd (8)).
Also, see the section on `Anonymous Connections' in the manual
.I netshell (8).
.H 1 "Network Process Management"
The day to day operation of the
network involves running many different programs.
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
file \*(sD\f(CW/_lib/initfile\fP.
Each program that is to be run by
.I netinit
has an entry of the form:
.P
.DS 1
\fIprocess-id\ \ \ process-type\ \ \ command\fP
.DE
.P
The
.I process-id
starts at the beginning of a line and is delimited by space, tab or newline.
It is used as an identifier for the process when
.I netinit
is asked to start or stop it by the
.I netcontrol (8)
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
changes directory to \*(sD before starting,
so pathnames can be relative to \*(sD,
or absolute.
.H 2 "Permanently Running Programs"
Some network programs need to be run continuously and restarted if they stop.
The routing program,
\*(sD\f(CW/_lib/router\fP
needs to be active at all times to process routing requests.
.P
.I Netinit
can be told to run and restart router by placing a line such as:
.P
.DS 1
.ft CW
.ta +\w'_lib/routerXX'u +\w'restartXX'u
router	restart	_lib/router -f
.ft
.DE
in
\*(sD\f(CW/_lib/initfile\fP.
.H 2 "Controlling Netinit"
.I Netinit
can be asked to start and stop processes or groups of processes,
re-read \f(CWinitfile\fP,
report the status of processes or shutdown all processes in an
orderly way using the
.I netcontrol
program.
.P
.I Netcontrol
takes a command name argument and an optional regular expression,
and carries out the requested operation on all processes whose process id's
match the regular expression.
.P
The commands are:
.P
.VL "\w'statusx[\fIregex\fP]xx'u+0.5c" 0.5c
.LI "status\ [\fIregex\fP]"
Report on the status of network processes with process id's that match
the regular expression \fIregex\fP.
Status of all network processes controlled by
.I netinit
will be reported if the regular expression is omitted.
.LI "start\ [\fIregex\fP]"
All network processes with id's matching \fIregex\fP
will be enabled.
.LI "curtail\ [\fIregex\fP]"
Network processes specified by \fIregex\fP
will be marked as not to be restarted.
.LI "stop\ [\fIregex\fP]"
Network processes specified by \fIregex\fP
will be sent a \s-1SIGTERMINATE\s0 and marked as not to be restarted.
.LI "kill\ [\fIregex\fP]"
Network processes specified by \fIregex\fP
will be sent a \s-1SIGKILL\s0 and marked as not to be restarted.
.LI shutdown
All network processes will be sent a \s-1SIGTERMINATE\s0 and
.I netinit
will also terminate.
.LI scan
.I Netinit
will re-read the initfile.
Active processes that are not in the new file will be \fIstopped\fP
and new processes will be noted but not enabled.
Processes for which the information (such as the command arguments) has changed
will have the new information included but it won't take effect until
the process is next restarted.
.LE
Netinit writes details of the activities it is controlling into the file
\*(sD\f(CW/_lib/log\fP.
.H 2 "Setting up periodic calls"
If you only (or also) wish to make
.I periodic
calls to some other node,
then you need to add an
.I initfile
entry to make the calls at the chosen intervals.
For instance,
to call the node ``sol'' at noon, 6pm, and midnight,
place appropriate commands in a shell_script
.br
\*(sD\f(CW/_call/call_sol\fP,
and add the line:
.P
.DS L
.ps -1
.ft CW
.ta +\w'call-sol.XYZ.MHS.auXX'u +\w'"0 0,12,18 * * *"XX'u
call-sol.XYZ.MHS.au	"0 0,12,18 * * *"	_call/call_sol
.ft
.ps
.DE
to
\*(sD\f(CW/_lib/initfile\fP.
Note that we specify the full untyped region name of the link,
so that this entry can be used by the default
.I auto-call
facility.
.P
As another example,
if we had a Hayes compatible dialup modem and wished to call
``wild'' at 7am and 7pm,
then we could add to
\*(sD\f(CW/_lib/initfile\fP
the entry:
.P
.DS L
.ps -1
.ft CW
.ta +\w'call-sol.XYZ.MHS.auXX'u +\w'"0 0,12,18 * * *"XX'u
call-wild.UVW.com.us "0 7,19 * * *" _lib/VCcall
	-S _call/hayes.cs # call script
	"-Dloginstr=altair"
	"-Dpasswdstr=secret"
	"-Dphoneno=T0,001112011234567"
	"-Dmodemdev=/dev/cua"
	"-Dlinespeed=2400"
	"-Ddmnargs=-D256 -X240"
	wild.UVW.com.us
.ft
.ps
.DE
.P
As it happens,
there are shell programs for the more common types of call,
(see
.I netcallers (8)),
and so this line could have been specified more simply as:
.P
.DS L
.ps -1
.ft CW
.ta +\w'call-sol.XYZ.MHS.auXX'u +\w'"0 0,12,18 * * *"XX'u
call-wild.UVW.com.us "0 7,19 * * *" _call/call.modem_0 -f
	"wild.UVW.com.us"
	"hayes@/dev/cua"
	"T0,001112011234567@2400"
	"altair"
	"secret"
.ft
.ps
.DE
.P
This entry could have been placed on one long line but we have
spread it over several lines for clarity.
Continuation lines of an entry in
.I initfile
are indicated by leading white space.
.P
Call script programs are provided in the \f(CW_call\fP directory for several
popular modems and other connection types.
For a new connection type you can easily write your own call script
program (see
.I netcall (8)).
.P
Periodic connections involve less overheads than
.I auto-call ,
and allow the scheduling of connections
to take advantage of cheaper billing periods.
.P
In most cases a new connection can be made to the network by adding one entry to
\*(sD\f(CW/_lib/initfile\fP.
.H 1 "Accounting Information"
If you don't want to gather accounting information,
then you should remove the file called:
.P
\*(sD\f(CW/_stats/Accumulated\fP.
.P
Otherwise, the network will log at least one statistics record in
.I Accumulated
for every message processed.
.P
The statistics are collected as lines containing
`\fB:\fP' separated \s-1ASCII\s0 fields.
The fields are defined in
.I netstats (5)
and printed by the command
.I netstatspr (8) .
This file can grow very quickly on busy systems,
so it is advisable to turn off statistics accumulation unless you have a
use for the detailed information.
.P
On the other hand, if you want to \fIturn on\fP accounting,
then you should just create this file with the right owner and permissions.
It should be writable by the same user that owns the directory it is created in.
.P
An automatic statistics aggregation command
is run once at the beginning of every month by
.I netinit (1)
(see \*(sD\f(CW/_stats/turn-acct\fP).
.H 2 "Connect Accounting"
The network transport daemons will optionally log
details for every connection with another site.
To select this facility,
you should create a file called:
.P
\*(sD\f(CW/_stats/connect\fP.
.P
It should be writable by the same user that owns the directory it is created in.
.P
The statistics are collected as lines containing
\fIspace\fP separated \s-1ASCII\s0 fields,
one line for each direction (ie: two per connection).
Each line in the connect file has 8 fields as follows:
start-date, start-time, weekday, direction, typed-link-name,
elapsed-time, messages and data-bytes.
NB: \fIuni-process\fP daemons write a single line
with the direction marked as \f(CWi&o\fI.
.P
Connect records are printed by the command
.I netcac (8) .
.H 1 "System Parameters"
Your distribution of \s+2\*(sN\s-2 comes preconfigured for your hardware and
standard Unix system.
If you wish to change system parameters such as the spool directory or
the mail delivery agent, then you must add
parameter specifications to the appropriate parameter file.
Some parameters that are relevant to the whole system can be redefined in
the file \f(CW/usr/lib/MHSnetparams\fP.
Others are redefined in files in the directory \*(sD\f(CW/_params\fP,
where \*(sD is normally \f(CW/usr/spool/MHSnet\fP, but can be redefined
in \f(CW/usr/lib/MHSnetparams\fP.
.H 2 "System Interfaces"
The parameters are to do with the network's interface
to your kernel and to the file system, and to programs invoked by the network.
Some parameters specify the names of programs,
and the format of any special arguments
that will need to be passed to them on invocation.
There are some macros that can be used inside arguments
to interpolate fields from the message being processed by the handler,
they are all identified by the special character \fB&\fP:
.P
.VL "\w'&%XX'u+.25i" .25i
.LI &D
.I "Data name" .
The name of the data in the message.*
.LI &E
.I "Error" .
The value of the error field in the message header.
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
Those marked with an asterisk (*) are only valid for messages containing
``File Transfer Protocol'' (\s-1FTP\s0) as generated by
.I netfile (1),
e.g. mail messages.
.P
There is another set of macros that can be used to interpolate a string
depending on the evaluation of a simple boolean expression.
If the string is not interpolated the macro is replaced by the null string,
and if this causes the entire argument to become empty it is
omitted from the list of arguments.
These macros are all enclosed by the special character \fB%\fP, and
take the form:
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
Selector characters (\fIX\fP) are:
.P
.VL "\w'&%XX'u+.25i" .25i
.LI A
True if the message is an acknowledgment.
.LI B
True if the message had a
.I broadcast
address.
.LI D
True if the message might have been duplicated.
.LI L
True if \fB&O\fP is not identical with \fB&L\fP,
i.e. if the message did not originate at the neighbour node.
.LI Q
True if the message has requested an acknowledgment.
.LI R
True if the message has been returned.
.LI T
True if the message has been truncated.
.LE
.P
Arguments should be specified just as though you were listing them inside
a C program function invocation, eg: to pass three arguments to a program,
the first interpolating ``-I'' if the message was not broadcast,
the second the name of the sender, and the third the name of her host,
you might specify:
.P
.DS 1
.ft CW
ARGS="%!B-I%","-u&F","-h&O"
.ft
.DE
.P
Note the use of double-quotes.
.bp
.P
Here are descriptions of the parameters.
After each description, the file in which the parameter is defined
is given in square brackets.
For a complete list of parameters and their default values
you should refer to the manual entry
.I netparams (5).
Note that parameters which are the names of network files
needn't have the ``\*(sD/'' specified,
because if the file name doesn't begin with a leading `\fB/\fP',
then \*(sD/ will be prepended.
.VL "\w'\s-1IGNMAILERSTATUS\s0xx'u"
.LI \s-1ACKMAIL\s0
Set this to \fB1\fP if you wish to turn on automatic acknowledgment for all mail generated by
.I netmail (1).
[\*(sD/_params/netmail]
.LI \s-1ADDRESS\s0
Set this to override the default `site' address for
.I netmail (1).
[\*(sD/_params/netmail]
.LI \s-1BINMAIL\s0
This defines the name of the program that will be used to deliver
mail generated internally by the network, eg: acknowledgments of
correctly delivered files, or error reports to the
.I NCC_MANAGER .
It must be able to read the body of the mail text from its standard input,
and to accept many user addresses as arguments
(after any special arguments mentioned in \s-1BINMAILARGS\s0 below).
\f(CW/bin/mail\fP
should always work,
as the network will terminate if it can't deliver a management message.
.br
[/usr/lib/MHSnetparams]
.LI \s-1BINMAILARGS\s0
If you have a version of mail that understands about interpolating the
origin host address in the mail item, then this is where you tell it what to do.
For example, a particular mail program may
use the argument ``\fB\-n\fP\fIuser\fP\fB@\fP\fIhost\fP''
to tell mail that we wish the item to appear to come
from someone other than the invoker;
ie: we set \s-1BINMAILARGS\s0 to be \f(CW"-n&F@&O"\fP.
.br
[/usr/lib/MHSnetparams]
.LI \s-1DFLTPARAMS\s0
The default parameters
to be used by the transport daemons if the connection fails to negotiate properly.
.br
[\*(sD/_params/daemon]
.LI \s-1FILESERVERLOG\s0
The name for an optional file which will be used to log fileserver requests.
Typically this would be \f(CW_lib/servedfiles\fP.
.br
[\*(sD/_params/fileserver]
.LI \s-1FILESERVER_MAXC\s0
The maximum number of columns allowed when listing pathnames.
If a path name is longer than this,
it is truncated at the front with the truncated part replaced by elipses.
[No truncation if \fB0\fP, the default is \fB80\fP.]
.br
[\*(sD/_params/fileserver]
.LI \s-1FILESERVER_MAXL\s0
The maximum size of a fileserver listing request that will be returned.
Long listings are truncated to this length.
[No truncation if \fB0\fP \(em the default.]
(It is suggested that a pre-prepared recursive directory listing
be placed in a file named
.if t \f(CWls-lRt.Z\fP
.if n `ls-lRt.Z'
to make it unnecessary for requesters to do recursive verbose listings.)
.br
[\*(sD/_params/fileserver]
.LI \s-1FILESERVER_NOLR\s0
Setting this parameter to \fB1\fP disables recursive listing requests to the fileserver.
.br
[\*(sD/_params/fileserver]
.LI \s-1FILESERVER_NOX\s0
Setting this parameter to \fB1\fP disables `in-place' file transmission by the fileserver.
Instead copies will be made of each file requested.
This is useful if files are being serviced that change frequently,
as it prevents the use of the \fB\-X\fP flag to 
.IR netfile (1),
which can cause the transport daemons to complain if an `in-place' file
changes size between being requested and being transmitted.
.br
[\*(sD/_params/fileserver]
.LI \s-1FILESEXPIREDAYS\s0
After files have been spooled at a site for delivery to a local user,
the user is notified by mail that the files have arrived,
and warned that the spooled files will be deleted
after a certain number of days.
This is where you specify that period.
It is effected by a periodic command run by
.I netinit .
[\*(sD/_params/filer]
.LI \s-1FILTERPROG\s0
Defines the name of the program to be run by a message
.I filter
to process the data from a message,
defaults to: \f(CWfiltername.sh\fP,
where
.I filtername
is the name the filter was invoked with.
.br
[\*(sD/_params/\fIfiltername\fP]
.LI \s-1FOREIGNUSERNAME\s0
Defines the user id that will own files passed to the foreign network interface,
defaults to: \s-1NETUSERNAME\s0.
[\*(sD/_params/\fIspoolername\fP]
.LI \s-1GETFILE\s0
The name of the program used by a user to get files they have been sent.
The normal value is \f(CW/usr/bin/netget\fP.
[\*(sD/_params/filer]
.LI \s-1HANDLERPROG\s0
Defines the name of the program to be run by a message
.I handler
to process the data from a message,
defaults to: \f(CWhandlername.sh\fP,
where
.I handlername
is the name the handler was invoked with.
[\*(sD/_params/\fIhandlername\fP]
.LI \s-1IGN_ETCPASSWD\s0
Setting this to \fB1\fP causes
.I netshell (8)
to ignore any entry in the
.ift \f(CW/etc/passwd\fP
.ifn ``/etc/passwd''
file and force a check of the network region password for a matching site (if set).
[\*(sD/_params/daemon]
.LI \s-1IGNMAILERSTATUS\s0
This parameter refers to the exit status of the program defined in
.SM MAILER
below.
Some delivery programs, notably
\f(CWsendmail\fP
from 4\s-1BSD\s0,
return non-zero exit status,
even when they have done something sensible with the mail item.
In this case, the handler would normally assume that something went wrong,
and return the mail item to its origin with an appropriate error message,
but you can suppress this action by defining this flag to be \fB1\fP.
If the mailer returns the exit status 67 [\s-1EX_NOUSER\s0]
as well as sending a separate reply for non-existent user errors,
then set this flag to \fB2\fP,
and only that error status will be ignored.
.br
[\*(sD/_params/mailer]
.LI \s-1KEEPSTATE\s0
This parameter controls the building of the site information data-base
used for quick reference by
.I netmap (1)
when showing the details for a particular site.
It causes the last state message received from each site to be saved
in a directory tree starting at
\*(sD/_state/MSGS.
Set it to \fB0\fP to prevent saving.
.br
[\*(sD/_params/stater]
.LI \s-1LOCALSEND\s0
Set this to 1 if you wish to allow users whose network \s-1SEND\s0 permission
has been denied in the ``netprivs'' file (see
.I netprivs (5))
to be able to
use the network to send messages to other users on the same node.
.br
[/usr/lib/MHSnetparams]
.LI \s-1LOCAL_NODES\s0
This is a more extensive version of \s-1LOCALSEND\s0, in that you can nominate
a set of nodes to which messages may be sent by users otherwise denied
network \s-1SEND\s0 permission.
It can either be a list of network addresses,
or the name of a file containing the list.
The addresses can be separated by <tab> or <newline>.
[/usr/lib/MHSnetparams]
.LI \s-1LOGFILE\s0
If a message filtering program defined in
\s-1FILTERPROG\s0 doesn't exist,
or is unexecutable,
then
.I netfilter (8)
defaults to logging information about the message
in the file defined by this parameter.
If this parameter is undefined,
it defaults to
\f(CW_stats/\fP\fIlink\fP\f(CW.fltr.log\fP
where
.I link
is the name of the link the filter is invoked for.
.br
[\*(sD/_params/\fIfiltername\fP]
.LI \s-1MAILER\s0
This defines a program that will be used to deliver mail
received off the network, ie: from users at other hosts.
It must behave in the same way as \s-1BINMAIL\s0 above,
and again,
\f(CW/bin/mail\fP
should always work.
However, if you have a special program used just for
.I delivering
mail, then this is the place to specify it.
[\*(sD/_params/mailer]
.LI \s-1MAILERARGS\s0
If \s-1MAILER\s0 can be told where the mail originated,
then it should be specified in this argument,
much the same as for \s-1BINMAILARGS\s0 above.
Please note that this is the only safe way to guarantee that
mail ``From'' lines are valid.
\s-1MAILER\s0 should interpolate this string in place of
any user/site addresses in the body of the mail item.
[\*(sD/_params/mailer]
.LI \s-1MAIL_FROM\s0
If your version of \s-1MAILER\s0 does not correct the ``From'' line
using the information from \s-1MAILERARGS\s0,
then setting this to \fB1\fP
will enable the mail handler itself
to add a standard \s-1UNIX\s0 mail header.
[\*(sD/_params/mailer]
.LI \s-1MAIL_RFC822_HDR\s0
Setting this to \fB1\fP causes internally generated mail to include an
RFC822 compliant mail header.
.br
[/usr/lib/MHSnetparams]
.LI \s-1MAIL_TO\s0
Setting this to \fB1\fP causes internally generated mail to include a
``To:\ '' header line.
.br
[/usr/lib/MHSnetparams]
.LI \s-1MAX_MESG_DATA\s0
This is the maximum message size that is sent through the network.
Messages larger than this are broken into \s-1MAX_MESG_DATA\s0 sized parts and
sent independently through the network and reassembled before delivery.
[/usr/lib/MHSnetparams]
.LI \s-1MESSAGEID\s0
Set this to \fB1\fP to cause
.I netmail (1)
to generate an RFC822 compliant
\f(CWMessage-ID: \fP
line in every header.
[\*(sD/_params/netmail]
.LI \s-1MINSPOOLFSFREE\s0
This is the minimum amount of free space desired on the network spool file-system (in kilobytes).
When the free space falls below this,
the message creating programs will either wait for more space to become available
(eg: the transport daemon receiver process \(em see
.I netdaemon (8)),
or return error codes (eg:
.I netfile (1)).
[/usr/lib/MHSnetparams]
.LI \s-1MKDIR\s0
The command for making a directory.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NCC_ADMIN\s0
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
.br
[/usr/lib/MHSnetparams]
.LI \s-1NCC_MANAGER\s0
In contrast to
\s-1NCC_ADMIN\s0,
this defines the name of the person to whom network generated information reports are sent.
Usually the same as
\s-1NCC_ADMIN\s0,
alternatively it may be the network address of
someone central who collects information on your branch of the network.
[/usr/lib/MHSnetparams]
.LI \s-1NETADMIN\s0
Set to \fB1\fP to cause mail notices of new regions added to routing tables
to be sent to \s-1NCC_MANAGER\s0.
Also causes notification of new/deceased links to your site.
Set it to \fB2\fP if you want
mail notices every time a new site sends its first state message.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NETGROUPNAME\s0
The group id used by network processes.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NETUSERNAME\s0
The user id used by network processes.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NEWSARGS\s0
Any arguments for \s-1NEWSEDITOR\s0.
.br
[\*(sD/_params/reporter]
.LI \s-1NEWSCMDS\s0
This should be the name of a file which is used by the news message handler
.I reporter
to provide the full pathname of a news command and any extra arguments.
This can safely be ignored until you become a sophisticated news gatherer,
but could be
.br
\f(CW_lib/newscmds\fP.
The file should contain lines of the form:
.P
.S -2
.DS
.ft CW
command<tab>full_path_name[<space>arg ...]
.ft
.DE
.S
You should consult with your news neighbours to determine
the necessary commands to install here (if any).
[\*(sD/_params/reporter]
.LI \s-1NEWSEDITOR\s0
If you participate in the
.I netnews
system,
this argument specifies the news receiver program to be invoked by the news message handler
.I reporter .
For the standard netnews software,
this should be set to
.ft CW
/usr/bin/rnews.
.ft
[\*(sD/_params/reporter]
.LI \s-1NEWSIGNERR\s0
Some versions of
.I rnews
return non-zero exit status when they receive a null news item.
If these are a frequent occurrence,
or you simply wish to ignore all delivery errors for news items,
then you can suppress the error reports by defining this flag to be \fB1\fP.
It is probably a good idea to leave the value at \fB0\fP (the default)
until you are sure that normal
news items are being delivered correctly.
.br
[\*(sD/_params/reporter]
.LI \s-1NICEDAEMON\s0
If you wish the transport daemons to run at a non-standard priority,
then this parameter should be defined as for use in the
.I nice (2)
system call.
For instance, if you wanted the network to run only when there was
idle \s-1CPU\s0 time, then give it the value 19.
However, be aware that if you do this when the system is busy,
then the links may appear to go up and down a lot.
Using a value of -11 will ensure the transport daemons
always achieve high throughput,
which is acceptable since they are not very \s-1CPU\s0 intensive.
A value of 0 is probably safe.
.br
[\*(sD/_params/daemon]
.LI \s-1NICEHANDLERS\s0
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
so slowing down a particular handler will also delay any following messages
(but see
.IR nethandlers (5)
for a way to specify asynchronous \fIsub\fP-routers for particular handlers).
[/usr/lib/MHSnetparams]
.LI \s-1NOADDRCOMPL\s0
Set to \fB0\fP to allow address completion.
Without address completion,
every network address must have all domains present,
up to and including the \s-1COUNTRY\s0-level domain,
or be fully typed.
If address completion is allowed,
then addresses ending in any untyped domain
that matches one in the local hierachy
will have the remainder of the hierarchy appended.
This may not be a good idea if any domain in the hierarchy
happens to be the same as a valid \s-1COUNTRY\s0 code,
or other domain in the routing tables.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NODENAMEFILE\s0
If your \s-1UNIX\s0 kernel is a variant of neither \s-1SYSTEM V\s0, nor \s-1BSD\s04.x, then
this parameter names a file where the name of your node is kept. This
name is used in displaying which nodes in a distributed file system the
various network daemons are running on. This is needed since the network
name of the collection of nodes in all probability will be different.
.br
[/usr/lib/MHSnetparams]
.LI \s-1NORET\s0
Setting this to \fB1\fP will cause
.I netmail
to request that all delivery errors be ignored for mail messages it generates.
[\*(sD/_params/netmail]
.LI \s-1NVETIMECHANGE\s0
The maximum negative system time change (in seconds)
tolerated by time sensitive daemons (default: 10 seconds).
.br
[\*(sD/_params/daemon \*(sD/_params/netinit]
.LI \s-1NVETIMEDELAY\s0
The time in seconds that
.I netinit (8)
will delay after detecting a negative system time change
greater than
\s-1NVETIMECHANGE\s0
before re-starting network processes (default: 600 \(em 10 minutes).
.br
[\*(sD/_params/netinit]
.LI \s-1PORT\s0
The port number used by \*(sN TCP/IP connections.
[\*(sD/_params/tcplisten]
.LI \s-1POSTMASTER\s0
This must be set to be a name that messages returned from your node
may legally be sent to in the case of the origin also returning them.
If your mail program doesn't recognise anything special such as
\s-1POSTMASTER\s0, then this must be the name of a real user.
International networking conventions require that
``postmaster''
be a valid mail address on your system as a way to
guarantee deliveries of enquiries and mail problem reports,
so the mail message handler intercepts any case of ``postmaster''
and interpolates the value of this parameter.
.br
[/usr/lib/MHSnetparams]
.LI \s-1PRINTER\s0
The name of the program used to spool print jobs from the network.
Leave it undefined if this host has no printer,
or if you have no intention of letting other sites print their jobs here.
(But see \s-1PRINTORIGINS\s0 below.)
[\*(sD/_params/printer]
.LI \s-1PRINTERARGS\s0
Any arguments for \s-1PRINTER\s0.
.br
[\*(sD/_params/printer]
.LI \s-1PRINTORIGINS\s0
If you \fBdo\fP define \s-1PRINTER\s0, then you will need to restrict
the sites from which you are prepared to accept print jobs.
This parameter can be either the path name of a file
containing a list of legal network origins for print jobs,
or just a list of sites specified as a multicast address.
[\*(sD/_params/printer]
.LI \s-1PRIVSFILE\s0
The name of a file where network users' privileges will be listed.
\f(CW_lib/privsfile\fP is the usual choice
(see
.I netprivs (5)).
[/usr/lib/MHSnetparams]
.LI \s-1PUBLICFILES\s0
Defines the directory (or name of a file containing directories)
where publicly accessible files are held.
(see the section entitled ``Fileserver'' below).
.br
[\*(sD/_params/fileserver]
.LI \s-1RECEIVED\s0
Setting this to \fB1\fP will cause
.I netmail
to generate an RFC822 compliant
\f(CWReceived: \fP line in every mail header.
[\*(sD/_params/netmail]
.LI \s-1RECEIVER\s0
Defines the name of the program to be run by a message
.I spooler
to process the data from a message,
defaults to: \f(CWspoolername.sh\fP,
where
.I spoolername
is the name the spooler was invoked with.
[\*(sD/_params/\fIspoolername\fP]
.LI \s-1RMDIR\s0
The command for removing a directory.
.br
[/usr/lib/MHSnetparams]
.LI \s-1SENDACKARGS\s0
Defines the args to be passed to \s-1SENDER\s0 to send a mail message with an acknowledgment request.
.br
[\*(sD/_params/netmail]
.LI \s-1SENDARGS\s0
Defines the args to be passed to \s-1SENDER\s0 to send a mail message.
[\*(sD/_params/netmail]
.LI \s-1SENDER\s0
Defines the name of the program to send a network mail message.
[\*(sD/_params/netmail]
.LI \s-1SERVER\s0
The name of the program used to receive \*(sN TCP/IP connections.
[\*(sD/_params/tcplisten]
.LI \s-1SERVERGROUP\s0
The group name to run the file service under
(see the section entitled ``Fileserver'' below).
.br
[\*(sD/_params/fileserver]
.LI \s-1SERVERUSER\s0
The user name to run the file service under
(see the section entitled ``Fileserver'' below).
.br
[\*(sD/_params/fileserver]
.LI \s-1SERVICE\s0
The name of the ``service'' used when establishing a TCP/IP connection.
[\*(sD/_params/tcplisten]
.LI \s-1SHELL\s0
The name of the command interpreter to be used by network ``shell' scripts
(default: \f(CW/bin/sh\fP).
.br
[/usr/lib/MHSnetparams]
.LI \s-1SHOW_ROUTE\s0
Set this flag to \fB1\fP to show the message route by adding
.I Received:
lines to mail headers.
.br
[\*(sD/_params/mailer]
.LI \s-1SPOOLDIR\s0
This should be the name of a directory
where all the network activity will take place.
It will contain all the messages that are
.I "in flight"
as well as the handler programs and network state files.
It should be somewhere safe,
where the integrity of the messages is least likely to be compromised,
(so try and avoid less than fully reliable disk drives.)
The usual choice would be:
\f(CW/usr/spool/MHSnet\fP.
.br
\fBNOTE:\fP if you change \*(sD from the default
\f(CW/usr/spool/MHSnet\fP then any other parameters containing
\*(sD must also be changed.
.br
[/usr/lib/MHSnetparams]
.LI \s-1STATE\s0
The pathname of the ``netstate'' command.
.br
[/usr/lib/MHSnetparams]
.LI \s-1STATERNOTLIST\s0
This parameter allows you to nominate a set of nodes whose state
messages will be ignored if they arrive more frequently than once an hour.
It can either be a list of network addresses, or the name of a file containing
the list. eg:
.br
\s-1\f(CWSTATERNOTLIST=*.su.oz.au\fP\s0
.br
The addresses in ``ignorelist'' can be separated by <tab> or <newline>.
[\*(sD/_params/stater]
.LI \s-1STOP\s0
The pathname of the ``netstop'' command.
.br
[/usr/lib/MHSnetparams]
.LI \s-1STTY\s0
The pathname of the ``stty'' command.
.br
[/usr/lib/MHSnetparams]
.LI \s-1SUN3\s0
Set this to 1 if you wish the `.au' to be stripped
from the end of mail addresses ending in `.oz.au'
(for backward compatibility with sites running the \*(sM software).
.br
[/usr/lib/MHSnetparams]
.LI \s-1TMPDIR\s0
The pathname of the directory to use for temporary files.
[/usr/lib/MHSnetparams]
.LI \s-1TRACEFLAG\s0
Turns on program tracing at a level between \fB1\fP and \fB9\fP.
[(any parameter file)]
.LI \s-1ULIMIT\s0
The size of the ``ulimit'' value used by network processes.
[/usr/lib/MHSnetparams]
.LI \s-1UUCPLCKDIR\s0
The name of the directory where ``uucp'' creates its lock files.
.br
[\*(sD/_params/daemon]
.LI \s-1UUCPLCKPRE\s0
The prefix for ``uucp'' lock file names,
normally \f(CWLCK..\fP, which is followed by the device name.
.br
[\*(sD/_params/daemon]
.LI \s-1UUCPLOCKS\s0
Set this to \fB1\fP to turn on ``uucp'' locking for device access.
[\*(sD/_params/daemon]
.LI \s-1UUCPLOWERDEV\s0
Set this to \fB1\fP if `uucp' lock files
must have the last char of the `tty' name forced to lower-case,
common in SCO versions where `tty' names have an upper-case char
to indicate dial-out copy of a modem line,
(default: copy of `tty' name in `/dev').
.br
[\*(sD/_params/daemon]
.LI \s-1UUCPMLOCKDEV\s0
Set this to \fB1\fP if `uucp' lock files
are named after the major/minor device numbers,
rather than the `tty' name in `/dev',
(default: copy of `tty' name in `/dev').
.br
[\*(sD/_params/daemon]
.LI \s-1UUCPSTRPID\s0
Set this to \fB1\fP if ``uucp'' lock files contain the locking process \s-1ID\s0 in \s-1ASCII\s0,
(default: binary integer).
.br
[\*(sD/_params/daemon]
.LI \s-1VALIDATEMAIL\s0
Some mail delivery programs are guaranteed to
fail
(dump a
.I core
file, or write a
.I dead.letter
file)
if the destination user doesn't exist, in which case the handler
might as well check first that the user exists in the password file.
You can force this check to happen by defining this flag to be \fB1\fP.
.br
[\*(sD/_params/mailer]
.LI \s-1VCDAEMON\s0
The name of the virtual circuit daemon,
defaults to \f(CW_lib/VCdaemon\fP.
[\*(sD/_params/daemon]
.LI \s-1WHOISARGS\s0
Any additional arguments you might want to pass to \s-1WHOISPROG\s0.
[\*(sD/_params/peter]
.LI \s-1WHOISFILE\s0
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
.DS
.ft CW
cut -d: -f1,5 /etc/passwd |
tr '[A-Z]' '[a-z]' >/usr/pub/whois
.ft
.DE
[\*(sD/_params/peter]
.LI \s-1WHOISPROG\s0
The name of the program which will attempt to match the supplied pattern
in the
.I whois
data-base.
It will be passed arguments as for
.I egrep (1),
which is the obvious candidate.
[\*(sD/_params/peter]
.LE
.H 1 "Network Privileges"
It is often necessary to restrict some users from using particular
facilities of the network.
For example some users may be allowed to receive messages but not send
them.
Other users may be restricted to sending messages to certain regions of
the network, within your organisation for example.
\*(sN has a facility that allows network privileges to be granted or
denied to users.
.P
The file \*(sD\f(CW/_lib/privsfile\fP
contains network permissions for people accessing the network.
Each line in the file describes the permissions to be allowed
for a particular person, or for a group.
.P
Lines start with a user or group name in the first field,
followed immediately by one colon (:) for a user,
or two colons for a group,
then a list of privileges separated by
.I space .
.I Space
is any combination of <tab>, <space>, `,', or `;'.
Privileges may be spread over several lines
by preceding the continuation lines with
.I space .
Any
.I space
character may be escaped by preceding it with the character (`\fB\e\fP').
Comments may be introduced with a hash (#),
and continue to end-of-line.
.P
An asterisk (*) in the first field allows default privileges to be set.
Subsequent default lists modify previous default lists.
.P
Named users inherit the current composite set of defaults.
Group entries set defaults for all users who are a member of that group
and have not been named explicitly before the group name occurs.
Any users not explicitly named in the file,
and whose group is not named,
will inherit the final composite of defaults.
.P
Each privilege listed is granted to the current list of users.
Alternatively a privilege may be removed by preceding it with either
a minus-sign (\-),
exclamation point (!),
or tilde (~).
Privileges that have been granted are enabled first,
and then denied privileges are removed,
except in the special case of ``\-*'' (see below),
when all current privileges are turned off,
before enabling the granted ones.
.P
A user's privileges consist of enabling flags and an optional
address restriction.
The address restriction prevents a user from sending messages
outside the regions named, and is introduced with an
.I at
sign (@).
.P
The flags can be written in upper or lower case and are as follows:
.VL "\w'Otherhandlersxx'u"
.LI "*"
All flags.
.LI "Broadcast"
Broadcast routing allowed.
.LI "Explicit"
Explicit routing allowed.
.LI "Multicast"
Multicast routing allowed.
.LI "Otherhandlers"
Message types not defined by
.I nethandlers (5)
are allowed.
.LI "Receive"
User is allowed to receive network messages.
So far only applies to the ability to use
.I netget (1).
.LI "Send"
User is allowed to send network messages.
.LI "SU"
This user is a network
.I "super-user" .
.LI "World"
Messages addressed to the top-level (global) region are allowed.
.LE
.ne 5
Here is a simple example:
.sp
.ft CW
.ps -2
.nf
.ta +\w'tom:X'u +\w'XXXX'u +\w'-OTHERHANDLERSXX'u
#	This is a comment line.
#	(Note that empty lines are ignored.)

*:		*	# Turn on everything for these:
root:
nuucp:
daemon:
fileserver:

*:		-WORLD	# Following are network super-users:
kre:
bob:

*:		-BROADCAST	# Mortals can't broadcast,
		-OTHERHANDLERS	# use weird handlers,
		-SU	# manipulate network messages,
		@COUNTRY=au	# or send messages outside the country.
jane:
tom:		+OTHERHANDLERS	# (except for Tom,
			# who is testing a new handler.)

#	Staff are restricted to particular networks.
staff::	@PRMD=MHS\e,PRMD=oz

#	The rest may only send and receive inside XYZ Pty Ltd.
*:		-*; SEND, RECEIVE, MULTICAST
		@ORG=XYZ.PRMD=MHS.COUNTRY=au
.fi
.ps +2
.ft
.P
One point to note is that if
.Cn privsfile
is present then the account \s-1SERVERUSER\s0 that is used by the
.Cn fetchfile
facility must be given
.Cn send
permission.
.H 1 "Interface to Mail"
Your distribution of \*(sN is configured to interface with your
standard mail system.
However, if you wish to use a non-standard mail system this manual
section tells you how to interface it to \*(sN.
.P
The distribution contains a mail program capable of using the
network to send mail to remote sites,
(see
.I netmail (1)),
and you can arrange for users to access this program
when they wish to send network mail.
Try modifying and making publicly available the shell program
.Cn _bin/netmailsh ,
which invokes the appropriate mail program depending on its arguments.
.P
You should arrange your mail insertion program to use the \*(sN
program
.I netfile (1)
to deliver messages.
You can use the command:
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
at one time to take advantage of \*(sN's
multi-cast addressing capabilities.
Note that the flags \fB-oP3\fP specify that the
message should be queued at a fixed,
time-ordered priority,
so that queued mail messages will be delivered in order.
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
file \*(sD\f(CW/_lib/handlers\fP (see
.I nethandlers (5))
to turn on the
.I restriction
flag for the mail handler.
.P
When mail is received by MHSnet it invokes the mail handler which in
turn invokes a program to deliver the mail item to a users mailbox.
This delivery program is specified in the \*(sD\f(CW/_params/mailer\fP
file using the \f(CWMAILER\fP and \f(CWMAILERARGS\fP variables.
See the section entitled ``System Parameters'' and the manual entry
.I netparams (5)
for details.
.P
As an example, if your system uses the Berkeley
.I sendmail
program for mail delivery, then the following parameter settings may suffice
(depending on your particular configuration of sendmail):
.P
.DS 1
.ft CW
MAILER="/usr/lib/sendmail"
MAILERARGS="-oem","-oit","-f&F@&O"
.ft
.DE
.P
These lines should be placed in the file \*(sD\f(CW/_params/mailer\fP.
.H 1 "Remote Printing"
If you wish to set up a remote printing facility,
the following example describes the parameters for printing to be allowed
between three hosts ``site1'', ``site2'' and ``site3'',
all running a version of
.I lpr (1)
with some added features.
Let us also assume that the network is being configured at ``site1''
which has a printer, and that ``site2''
has two printers.
.P
Let us assume that your line printer spooler program is called
.I lpr
and it takes the arguments:
\fB\-h\fP\fIhost\fP to indicate that the job comes from a remote site called
.I host ,
\fB\-u\fP\fIperson\fP to indicate that the job comes from a remote user called
.I person ,
\fB\-n\fP\fIname\fP to indicate the name of the print job,
and \fB\-p\fP\fIprinter\fP to indicate an alternative printer to print the job on.
\*(sN will pipe the contents of a
.I print
message into the print spooler program.
The name of this program is specified by setting the parameter
\s-1PRINTER\s0, in the file \*(sD\f(CW/_params/printer\fP.
In our case we should set the parameter \s-1PRINTER\s0 to be
.I /bin/lpr .
That is, in the file
.br
\*(sD\f(CW/_params/printer\fP
.br
we place the line
.P
.DS 1
.ft CW
PRINTER = "/bin/lpr"
.ft
.DE
.P
In a similar way, the arguments for the print spooler are specified
in the parameter \s-1PRINTERARGS\s0.
In our example we will set \s-1PRINTERARGS\s0 to show the true name
and origin of the job, ie:
.P
.DS 1
.ft CW
PRINTERARGS = "-h&O","-u&F","-n&D"
.ft
.DE
.P
Now we should define \s-1PRINTORIGINS\s0 to restrict print jobs to
our own sites, so we set
.P
.DS 1
.ft CW
PRINTORIGINS = "/usr/lib/printorigins"
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
That's all for the parameters,
but we also need to make it easy
for our users to send their print jobs to ``site2''
and be able to choose what printer it goes on.
This can be done by writing a shell script called, say
\f(CW/usr/bin/netprint\fP.
It could contain the following:
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
.H 1 "Fileserver"
If you so desire, you can make your host available to
others to access files that you might happen to have,
and which are generally available to all.
These files are accessed by the network program
.I netfetch (1)
whose requests are serviced by the message handler ``fileserver''.
.P
If you are going to do this,
then you should define the parameter \s-1PUBLICFILES\s0
to be the name of a file or directory that
will give access to the files you will be making
available (in \*(sD\f(CW/_params/filer\fP). 
If \s-1PUBLICFILES\s0 is the name of a directory,
then you should create it.
It can be owned by the user id defined in the parameter \s-1SERVERUSER\s0
to act on behalf of remote users.
If you have any files that you wish to make immediately
available, you can copy, or link, them into this
directory.
Make sure that they are readable by \s-1SERVERUSER\s0!
It's fine to make subdirectories if there is a lot
of related material that should be kept together.
.P
If \s-1PUBLICFILES\s0 is the name of a file, then
you need to create that file and place in it a set of lines that
describe the access permissions.
Each line has the format:
.P
.DS 1
.ta \w'friend@neighbourMM'u
[\fIusers\fP@]\fIaddress\fP	\fIdirectory\fP
.DE
The
.I users
can be a user name or an asterisk indicating any user name.
Similarly, the address can be an asterisk to indicate requests from
anywhere.
.P
A minimal case (and probably the best starting
point) would be to place the following lines in the access control
file:
.P
.DS 1
.ft CW
.ta \w'friend@neighbourMM'u
*	/usr/spool/MHSnet/_public
.ft
.DE
.P
This allows anyone to access files in /usr/spool/MHSnet/_public
and is equivalent to simply defining \s-1PUBLICFILES\s0 to
be this directory name.
However, it does leave room for expansion, should you
wish to at some later time.
.P
.I Fileserver
reads this file, forwards, looking for a match of user and address
with those of the user who requested files.
The first line that matches (ie: the first line for
which either the 
.I <user> 
part is the name of the remote user, or is missing or *, and the 
.I <address> 
part either matches the name of the remote host, or is *)
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
.ta \w'friend@*.XYZ.MHS.auMM'u
network@*.XYZ.MHS.au	/usr/spool/MHSnet
friend@neighbour	/
enemy@*
*	/usr/spool/MHSnet/_public
.ft
.DE
.P
This allows requests from the user \fInetwork\fP
at any site in the organisation XYZ
to access publicly readable files in your spool directory
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
is in your group file.
.P
If you wish, the
.I fileserver
handler will keep statistics of all files fetched from your site.
You should define the \s-1FILESERVERLOG\s0 parameter,
and make sure the file exists and is writable by \s-1SERVERUSER\s0,
as otherwise the handler will quietly avoid producing any statistics.
.H 1 "Network News"
This section describes the configuration that will allow \*(sN to carry news articles.
.H 2 "Requirements"
There are two major news systems available, B News and C News.
One or other of these news systems must be present.
.H 2 "Sending News"
News articles are transmitted by the news software using the 
.I netmsg
command.
An entry in the news `sys' file to send individual articles to the
system `alpha' might look like:
.P
.DS L
.ps -1
.ft CW
alpha:world,comp,news,gnu,aus,to.alpha:S:\e
  \s-2/usr/spool/MHSnet/_bin/netmsg -dorxP9 -Areporter -Z600000 -Dalpha\s0
.ft R
.ps
.DE
.P
This arranges for news articles (which are given to the command on
standard input) to be sent to the 
.I reporter
handler at `alpha'.
The \fBd\fP option prevents 
.ft CW
netmsg
.ft
from triggering the 
.I auto-call
mechanism when the news is queued.
The \fBo\fP option has the effect of
keeping news articles ordered.
The \fBr\fP option prevents news from
being returned to your host if (for some reason) it is not acceptable at
your neighbour node.
News is seldom important enough to worry about the
small chance of its being rejected.
The \fBx\fP option causes this message's transport costs to be charged to the recipient.
The \fBP9\fP option causes 
.I netmsg
to treat news as the lowest possible priority traffic.
The \fBA\fP option specifies that the message should be processed by the
.I reporter
handler at the destination.
The \fBZ\fP option specifies that if the
news hasn't been delivered within about one week,
then it should be removed.
Finally, the \fBD\fP option specifies `alpha' as the destination machine.
.P
Note that
.IR netmsg ,
not
.I netfile
is used.
This is because the
.I reporter
handler does not use the File Transfer Protocol.
.P
Also note that the destination
need not be directly connected to your host.
Any addressable host is a valid target for
.IR netmsg .
.P
Compressed news batches are also sent using a 
.I netmsg
command similar to the above.
In this case,
.I netmsg
is usually invoked within a 
.I sendbatch
shell script that collects the articles, compresses them and sends them.
If you are sending to more than one site, you should take advantage of
\*(sN's multi-casting ability.
.H 2 "Receiving News"
When a news article or compressed news batch is received by \*(sN
it is processed by the
.I reporter
handler.
.IR Reporter ,
in turn, passes the article or batch to a program from the news system.
In the standard \*(sN configuration this program is
``/usr/bin/rnews''.
If this must be changed, the parameter \s-1NEWSEDITOR\s0 can be set
to the pathname of the required program in the file
``\s-1SPOOLDIR\s0/_params/reporter''
The two major news systems available, B News and C News, both use
.I rnews
as the news-spooling program, hence the
recommendation that \s-1NEWSEDITOR\s0 have the value
.IR /usr/bin/rnews .
.P
The parameter \s-1NEWSCMDS\s0 is also available for controlling how
incoming batches are handled, but it is really only for backward
compatibility, and meant to give more control over the handling of
incoming news, since in older news systems, methods of unbatching and
decompressing were not standardised and not done by one program.
This is all now standardised and handled by
.IR rnews ,
so you should not need it.
.H 2 "News parameters"
If you wish to receive and/or distribute network news,
there are four system parameters
that you may need to change,
viz. \s-1NEWSARGS\s0, \s-1NEWSCMDS\s0, \s-1NEWSEDITOR\s0, \s-1NEWSIGNERR\s0;
for details of which, see the \f(HBManagement Guide\fP section on system parameters (section 6).
.P
\s-1NEWSCMDS\s0 and, \s-1NEWSEDITOR\s0 have already been mentioned.
\s-1NEWSARGS\s0 is available if a news receiver other than
.I rnews
is specified,
and it requires that certain arguments be specified when invoked.
\s-1NEWSIGNERR\s0 is available if other sites are sending incorrectly formed news articles,
and you wish to ignore the errors returned by
.IR rnews .
In which case set it to \fB1\fP.
.if \n(S3 \{\
.H 1 "SUN III"
\*(sN is configured to interface with the old version of the software
known as \*(sM.
This is achieved by specifying gateway programs
to convert the messages between the different formats.
A complete set of \*(sM compatible transport daemons are provided
for \fIlink-level\fP compatibility between the two networks.
There is also a \fIfilter\fP program for transforming the messages in each direction.
And finally,
there is a program that can be invoked directly
by the \*(sM software to
act as a \fIlink spooler\fP
to pass inbound messages from a running \*(sM network
across to \*(sN.
.H 2 "Connecting to sites running \*(sM"
This requires that the link use one of the \*(sM compatible daemons,
and the filter program ``\*(sD/_lib/filter43''.
Read the \*(sM sections in the manuals for the transport daemon (\c
.I netdaemon (8))
and the circuit accepting program (\c
.I netshell (8))
to be certain of what you are doing.
However,
the two main connection types are catered for in the \*(sN configuration program.
When the ``configmhs'' program asks you:
.P
.DS 1
.ps -1
.ft CW
Is this a connection to an ACSnet site with SUN III?
.ft
.ps
.DE
.P
just answer \f(CWyes\fP.
.P
Note that your visible region may need to be changed from the default
(\s-1ORGANISATION\s0-level).
\*(sM requires that at least one node in a site on the net must be
in the top-level (``oz'') domain so that all other sites may find routes to it.
So when the configuration program asks you:
.P
.DS 1
.ft CW
Visible region (<ORG> or oz)? [default: <ORG>]
.ft
.DE
.P
just answer \f(CWoz\fP.
Only one of your nodes \(em the one connected to the outside world \(em needs to be in ``oz''.
.H 2 "Converting a site that was running \*(sM"
If your site was running \*(sM before \*(sN was installed,
then the major task is to add all the old links into your \*(sN configuration.
.P
If all the old links are empty of messages,
then your task is simple \(em
just disable the old software, and start \*(sN.
If, on the other hand, some of the old links have messages queued for them,
then you must continue to run the old software until the queues have drained.
This is achieved by specifying a \fIlink spooler\fP for each \*(sM link,
so that incoming messages are passed over to the new software,
while the outgoing message queues are allowed to drain.
The spooler is called ``\*(sD/_lib/Sun3_4''
and should be specified for every link in your \*(sM ``commandsfile'', eg:
.P
.DS 1
.ft CW
spooler \fIlinkname\fP /usr/spool/MHSnet/_lib/Sun3_4
.ft
.DE
.P
When the old queues are empty,
disable \*(sM,
and start making the connections with \*(sN.
.H 2 "Running \*(sN alongside \*(sM"
Alternatively, if you can consider the option of bringing up \*(sN with a new nodename,
then you can run the two networks side-by-side on the same machine,
with a local link connecting the two ``nodes''.
This is very much more efficient if you have many links,
since there is effectively only one link on which messages must be transformed,
and then broadcast messages have much lower overheads.
The local link can use \s-1UDP/IP\s0, and be very fast and relatively efficient.
In which case you specify in your \*(sN ``commandsfile''
(in the directory ``_state'')
that the link from \*(sN to \*(sM be filtered as follows:
.P
.DS 1
.ft CW
filter \fIsun3nodename\fP _lib/filter43
.ft
.DE
.P
The \*(sM-compatible ENdaemon is started at each end
by using the \*(sN call script ``_call/call.udp3''.
.H 2 "Using UDP/IP over AARN"
Some sites are using \s-1UDP/IP\s0 links (using ``ENdaemon'') over AARN.
This is unwise,
since the ENdaemon protocol was designed specifically for \s-1UDP\s0 links
\fBwhere the \s-1UDP\s0 packets are travelling over an Ethernet\fP.
If the transport medium does not guarantee data-integrity,
the ENdaemon protocol becomes unreliable.
Instead, you are advised to use NNdaemon over TCP/IP,
with the added benefit that the smaller packets don't
conflict quite so much with interactive traffic.
\}
.TC
.CS
