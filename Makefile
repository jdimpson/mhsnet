#
#	Copyright 2012 Piers Lauder

#	This file is part of MHSnet.

#	MHSnet is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.

#	MHSnet is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.

#	You should have received a copy of the GNU General Public License
#	along with MHSnet.  If not, see <http://www.gnu.org/licenses/>.

#
#	=======================================================
#	The following macro definitions are all site-dependent.
#	The definitions will be placed in the include files
#	"$(INC)/site.h", and "$(BIN)/strings-data.h".
#	=======================================================
#
#	For suitable settings for the following macro definitions,
#	please refer to the installation guide in "Documents/install.mm".
#
BINMAIL		= /bin/mail
#BINMAILARGS	= \"-f&F@&O\"
CHECK_LICENCE	= 1
CRCS_O		= crcs.o
FILESERVERLOG	= $(STATSDIR)/servedfiles
FILESEXPIREDAYS	= 28
IGNMAILERSTATUS	= 0
INSBIN		= $(ISPOOLDIR)/_ubin
KEEPSTATE	= 1
LOCALSEND	= 1
#LOCAL_NODES	= $(LIBDIR)/localnodes
MAILER		= /bin/mail
#MAILERARGS	= \"-f&F@&O\"
MAIL_FROM	= 1
MAIL_RFC822_HDR	= 1
MAIL_TO		= 1
MANTYPE		= T
MAXRETSIZE	= 10000
MAX_MESG_DATA	= 2097152L
NCC_ADMIN	= root
NCC_MANAGER	= root
MINSPOOLFSFREE	= 100
NETADMIN	= 1
NETGROUPNAME	= daemon
NETPARAMS	= /usr/lib/MHSnetparams
NETUSERNAME	= daemon
#NEWSARGS	=
#NEWSCMDS	= $(LIBDIR)/newscmds
NEWSEDITOR	= /usr/bin/rnews
NEWSIGNERR	= 0
NICEDAEMON	= -1
NICEHANDLERS	= 1
NOADDRCOMPL	= 1
#NODENAMEFILE	= /etc/whoami
NVETIMECHANGE	= 10
NVETIMEDELAY	= 600
PASSWDSORT	= \"sort -t. -k2nr %s -o %s\"
POSTMASTER	= root
POSTMASTERNAME	= Postmaster
PRINTER		= /usr/bin/lp
PRINTERARGS	= \"-t&F@&O\",\"-s\"
#PRINTORIGINS	= $(LIBDIR)/printorigins
PRIVSFILE	= $(LIBDIR)/privsfile
PROTO_STATS	= 1
PUBLICFILES	= $(LIBDIR)/publicfiles
SERVERGROUP	= nuucp
SERVERUSER	= nuucp
SHOW_ROUTE	= 0
SMTP		= 0
SPOOLDIR	= /usr/spool/MHSnet
ISPOOLDIR	= $(SPOOLDIR)
#STATERNOTLIST	= $(STATEDIR)/ignorefile
SU		= root
SUN3		= 1
#SUN3LIBDIR	= _lib
#SUN3SPOOLDIR	= /usr/spool/ACSnet
#SUN3STATEP	= /usr/local/acsstate
#SUN3USERNAME	= daemon
#SUN3WORKDIR	= _work
TMPDIR		= /tmp
UUCPLCKDIR	= /usr/spool/uucp
UUCPLCKPRE	= LCK..
UUCPLOCKS	= 1
UUCPLOWERDEV	= 0
UUCPMLOCKDEV	= 0
UUCPSTRPID	= 0
VALIDATEMAIL	= 0
VCDAEMON_STATS	= 1
VERSION		= #	1.3 #0 92/07/30 CPU-OP.SYS.VN	(from "run.*")
WHOISARGS	= \"-i\",\"-e\"
WHOISFILE	= /usr/pub/whois
WHOISPROG	= /usr/bin/egrep
WORKFLAG	= _
#
#	Conditional compilation control for local system peculiarities.
#	Some of these are inter-dependent on most systems, but are isolated
#	additions on other systems.
#
AUSAS		= 0
AUTO_LOCKING	= 0
BSD4		= 0
BSD4V		= 0
CARR_ON_BUG	= 1
FCNTL		= 1
FLOCK		= 0
IPLIBS		=
KILL_0		= 1
LOCKF		= 1
MINSLEEP	= 1
NDIRLIB		= 2
PGRP		= 1
RENAME_2	= 1
SYS_LOCKING	= 0
SYSLIBS		=
SYSTEM		= 5
SYSVREL		= 3
TERMLIB		= -ltermcap
TCP_IP		= 1
UDP_IP		= 1
V8		= 0
VPRINTF		= 1
WAIT_RESTART	= 0
X25		= 0
#
#	Kernel peculiarities
#
FILEBUFFERSIZE	= 8192
#
#	Choose what you want the publicly visible commands to be called:
#	(and don't forget to change the names of the manual entries.)
#
FETCHFILE	= netfetch
FORWARD		= netforward
GETFILE		= netget
LINKSTATS	= netlink
MAPP		= netmap
QUEUE		= netq
SENDFILE	= netfile
SENDMAIL	= netmail
STATEP		= netstate
STOP		= netstop
WHOIS		= netwhois
#
#	Compilation control parameters
#
DEBUG		=
CFLAGS		= -O
LDFLAGS		=
CONFIG		=
#
#	Site dependent installation support program names
#
AR		= ar ru
AR_SUFX		= .a
CHGRP		= chgrp
CHMOD		= chmod
#				Remove 't' if you don't want sticky bits
CHMODMODE	= u=rwxt
#				Remove 't' if you don't want sticky bits
CHMODSUMODE	= u=rwxts
#				To remove `t' from above where not needed
CHMODMODE_T	= ,u-t
CHOWN		= chown
CP		= cp
ECHO		= echo
LORDER		= lorder
LN		= ln
MAKE		= make
MKDIR		= /bin/mkdir
#				Should be program to make a locking file
MKLOCK		= $(TOUCH)
#				Second argument for MKLOCK
MKLOCKTYPE	=
MV		= mv
RANLIB		= ranlib
RM		= /bin/rm -f
RMDIR		= /bin/rmdir
SHELL		= /bin/sh
STRIP		= strip
STTY		= /bin/stty
TSORT		= tsort
TOUCH		= touch
#
#	Site dependent name for Makefile.
#
SITE_DEPEND	= Makefile
#
#	Definitions for printing the source.
#
#	PRSPOOL assumes that PRSPOOLER needs the module name tacked
#	on the end (as in "lpr -iMHSnet"). If your print spooler can't
#	take such a parameter, then you can try using something like
#	"PRSPOOL=lpr" as an argument to make.
#
NAME		= MHSnet
PRSPOOLER	= apr -m$(NAME) -i
PRSPOOL		= $(PRSPOOLER)$(NAME)
PRFORMATTER	= pr -l66 -w132 -f -n4
#
#	============================================================
#	End of site-dependent definitions
#	============================================================

BIN		= Bin
CALLER		=
CONFIGDIR	= Config
DIRLIB		= Dirlib
FTHEADER	= FtHeader
HEADER		= Header
INC		= Include
LIB		= Lib
LIBC		= Libc
ROUTE		= Route
STATE		= State
VCLIB		= VClib

BADDIR		= $(WORKFLAG)bad
BINDIR		= $(WORKFLAG)bin
CALLDIR		= $(WORKFLAG)call
CONFDIR		= $(WORKFLAG)config
EXPLAINDIR	= $(WORKFLAG)explain
FILESDIR	= $(WORKFLAG)files
FORWARDINGDIR	= $(WORKFLAG)forward
HANDLERSDIR	= $(WORKFLAG)handlers
LIBDIR		= $(WORKFLAG)lib
MPMSGSDIR	= $(WORKFLAG)messages
PARAMSDIR	= $(WORKFLAG)params
PENDINGDIR	= $(WORKFLAG)pending
REROUTEDIR	= $(WORKFLAG)reroute
ROUTINGDIR	= $(WORKFLAG)route
STATSDIR	= $(WORKFLAG)stats
STATEDIR	= $(WORKFLAG)state
TRACEDIR	= $(WORKFLAG)trace
WORKDIR		= $(WORKFLAG)work

SLIBS		= $(SYSLIBS) -lc ../$(BIN)/$(LIBC)$(AR_SUFX)

SOURCES		= \
		$(INC) \
		$(DIRLIB) \
		$(FTHEADER) \
		$(HEADER) \
		$(LIB) \
		$(LIBC) \
		$(ROUTE) \
		$(STATE) \
		VCdaemon \
		HTdaemon \
		Admin \
		$(CONFIGDIR) \
		Control \
		Documents \
		Handlers \
		Init \
		Manuals \
		NNdaemon \
		Utilities \
		VCsetup

MYMAKEDEFS	= \
		-f ../Makefiles/Makefile0 -f Makefile \
		AR='$(AR)' \
		AR_SUFX='$(AR_SUFX)' \
		AS='$(AS)' \
		BIN='$(BIN)' \
		CC='$(CC)' \
		CFLAGS='$(CFLAGS)' \
		CONFIG='$(CONFIG)' \
		CRCS_O='$(CRCS_O)' \
		DEBUG='$(DEBUG)' \
		DIRLIB='$(DIRLIB)' \
		FRC='$(FRC)' \
		FTHEADER='$(FTHEADER)' \
		HEADER='$(HEADER)' \
		INC='$(INC)' \
		IPLIBS='$(IPLIBS)' \
		LDFLAGS='$(LDFLAGS)' \
		LIB='$(LIB)' \
		LIBC='$(LIBC)' \
		LN='$(LN)' \
		LORDER='$(LORDER)' \
		MAKE='$(MAKE)' \
		MKDIR='$(MKDIR)' \
		MV='$(MV)' \
		RANLIB='$(RANLIB)' \
		RM='$(RM)' \
		ROUTE='$(ROUTE)' \
		SHELL='$(SHELL)' \
		STATE='$(STATE)' \
		SLIBS='$(SLIBS)' \
		TERMLIB='$(TERMLIB)' \
		TSORT='$(TSORT)'

SITEFLAGS1	= \
		AUSAS='$(AUSAS)' \
		AUTO_LOCKING='$(AUTO_LOCKING)' \
		BADDIR='$(BADDIR)' \
		BIN='$(BIN)' \
		BINMAIL='$(BINMAIL)' \
		BINMAILARGS='$(BINMAILARGS)' \
		BSD4='$(BSD4)' \
		BSD4V='$(BSD4V)' \
		CALLDIR='$(CALLDIR)' \
		CARR_ON_BUG='$(CARR_ON_BUG)' \
		CHECK_LICENCE='$(CHECK_LICENCE)' \
		CHMOD='$(CHMOD)' \
		EXPLAINDIR='$(EXPLAINDIR)' \
		FCNTL='$(FCNTL)' \
		FILEBUFFERSIZE='$(FILEBUFFERSIZE)' \
		FILESDIR='$(FILESDIR)' \
		FILESERVERLOG='$(FILESERVERLOG)' \
		FILESEXPIREDAYS='$(FILESEXPIREDAYS)' \
		FLOCK='$(FLOCK)' \
		FORWARDINGDIR='$(FORWARDINGDIR)' \
		GETFILE='$(GETFILE)' \
		HANDLERSDIR='$(HANDLERSDIR)' \
		IGNMAILERSTATUS='$(IGNMAILERSTATUS)' \
		INC='$(INC)' \
		KEEPSTATE='$(KEEPSTATE)' \
		KILL_0='$(KILL_0)' \
		LIBDIR='$(LIBDIR)' \
		LOCALSEND='$(LOCALSEND)' \
		LOCAL_NODES='$(LOCAL_NODES)' \
		LOCKF='$(LOCKF)' \
		MAILER='$(MAILER)' \
		MAILERARGS='$(MAILERARGS)' \
		MAIL_FROM='$(MAIL_FROM)' \
		MAIL_RFC822_HDR='$(MAIL_RFC822_HDR)' \
		MAIL_TO='$(MAIL_TO)' \
		MAPP='$(MAPP)' \
		MAXRETSIZE='$(MAXRETSIZE)' \
		MAX_MESG_DATA='$(MAX_MESG_DATA)' \
		MINSLEEP='$(MINSLEEP)' \
		MINSPOOLFSFREE='$(MINSPOOLFSFREE)' \
		MKDIR='$(MKDIR)' \
		MPMSGSDIR='$(MPMSGSDIR)' \
		MV='$(MV)' \
		NCC_ADMIN='$(NCC_ADMIN)' \
		NCC_MANAGER='$(NCC_MANAGER)' \
		NDIRLIB='$(NDIRLIB)' \
		NETADMIN='$(NETADMIN)' \
		NETGROUPNAME='$(NETGROUPNAME)' \
		NETPARAMS='$(NETPARAMS)' \
		NETUSERNAME='$(NETUSERNAME)' \
		NEWSARGS='$(NEWSARGS)' \
		NEWSCMDS='$(NEWSCMDS)' \
		NEWSEDITOR='$(NEWSEDITOR)' \
		NEWSIGNERR='$(NEWSIGNERR)' \
		NICEDAEMON='$(NICEDAEMON)' \
		NICEHANDLERS='$(NICEHANDLERS)'

SITEFLAGS2	= \
		NOADDRCOMPL='$(NOADDRCOMPL)' \
		NODENAMEFILE='$(NODENAMEFILE)' \
		NVETIMECHANGE='$(NVETIMECHANGE)' \
		NVETIMEDELAY='$(NVETIMEDELAY)' \
		PARAMSDIR='$(PARAMSDIR)' \
		PENDINGDIR='$(PENDINGDIR)' \
		PGRP='$(PGRP)' \
		PASSWDSORT='$(PASSWDSORT)' \
		POSTMASTER='$(POSTMASTER)' \
		POSTMASTERNAME='$(POSTMASTERNAME)' \
		PRINTER='$(PRINTER)' \
		PRINTERARGS='$(PRINTERARGS)' \
		PRINTORIGINS='$(PRINTORIGINS)' \
		PRIVSFILE='$(PRIVSFILE)' \
		PROTO_STATS='$(PROTO_STATS)' \
		PUBLICFILES='$(PUBLICFILES)' \
		RENAME_2='$(RENAME_2)' \
		REROUTEDIR='$(REROUTEDIR)' \
		RM='$(RM)' \
		RMDIR='$(RMDIR)' \
		ROUTINGDIR='$(ROUTINGDIR)' \
		SENDFILE='$(SENDFILE)' \
		SERVERGROUP='$(SERVERGROUP)' \
		SERVERUSER='$(SERVERUSER)' \
		SHELL='$(SHELL)' \
		SHOW_ROUTE='$(SHOW_ROUTE)' \
		SPOOLDIR='$(SPOOLDIR)' \
		STATEDIR='$(STATEDIR)' \
		STATEP='$(STATEP)' \
		STATERNOTLIST='$(STATERNOTLIST)' \
		STATSDIR='$(STATSDIR)' \
		STOP='$(STOP)' \
		STTY='$(STTY)' \
		SUN3='$(SUN3)' \
		SUN3LIBDIR='$(SUN3LIBDIR)' \
		SUN3SPOOLDIR='$(SUN3SPOOLDIR)' \
		SUN3STATEP='$(SUN3STATEP)' \
		SUN3USERNAME='$(SUN3USERNAME)' \
		SUN3WORKDIR='$(SUN3WORKDIR)' \
		SYSTEM='$(SYSTEM)' \
		SYSVREL='$(SYSVREL)' \
		SYS_LOCKING='$(SYS_LOCKING)' \
		TCP_IP='$(TCP_IP)' \
		TMPDIR='$(TMPDIR)' \
		TRACEDIR='$(TRACEDIR)' \
		UDP_IP='$(UDP_IP)' \
		UUCPLCKDIR='$(UUCPLCKDIR)' \
		UUCPLCKPRE='$(UUCPLCKPRE)' \
		UUCPLOCKS='$(UUCPLOCKS)' \
		UUCPLOWERDEV='$(UUCPLOWERDEV)' \
		UUCPMLOCKDEV='$(UUCPMLOCKDEV)' \
		UUCPSTRPID='$(UUCPSTRPID)' \
		V8='$(V8)' \
		VALIDATEMAIL='$(VALIDATEMAIL)' \
		VCDAEMON_STATS='$(VCDAEMON_STATS)' \
		VERSION='$(VERSION)' \
		VPRINTF='$(VPRINTF)' \
		WAIT_RESTART='$(WAIT_RESTART)' \
		WHOISARGS='$(WHOISARGS)' \
		WHOISFILE='$(WHOISFILE)' \
		WHOISPROG='$(WHOISPROG)' \
		WORKDIR='$(WORKDIR)' \
		WORKFLAG='$(WORKFLAG)' \
		X25='$(X25)'

IFLAGS		= \
		BADDIR='$(BADDIR)' \
		BIN='$(BIN)' \
		BINDIR='$(BINDIR)' \
		CALLDIR='$(CALLDIR)' \
		CHGRP='$(CHGRP)' \
		CHMOD='$(CHMOD)' \
		CHMODMODE='$(CHMODMODE)' \
		CHMODSUMODE='$(CHMODSUMODE)' \
		CHMODMODE_T='$(CHMODMODE_T)' \
		CHOWN='$(CHOWN)' \
		CP='$(CP)' \
		CONFDIR='$(CONFDIR)' \
		CONFIGDIR='$(CONFIGDIR)' \
		ECHO='$(ECHO)' \
		EXPLAINDIR='$(EXPLAINDIR)' \
		FETCHFILE='$(FETCHFILE)' \
		FILESDIR='$(FILESDIR)' \
		FILESERVERLOG='$(FILESERVERLOG)' \
		FORWARD='$(FORWARD)' \
		FORWARDINGDIR='$(FORWARDINGDIR)' \
		FRC='$(FRC)' \
		GETFILE='$(GETFILE)' \
		HANDLERSDIR='$(HANDLERSDIR)' \
		INSBIN='$(INSBIN)' \
		ISPOOLDIR='$(ISPOOLDIR)' \
		LIBDIR='$(LIBDIR)' \
		LINKSTATS='$(LINKSTATS)' \
		LN='$(LN)' \
		MAPP='$(MAPP)' \
		MKDIR='$(MKDIR)' \
		MKLOCK='$(MKLOCK)' \
		MKLOCKTYPE='$(MKLOCKTYPE)' \
		MPMSGSDIR='$(MPMSGSDIR)' \
		MV='$(MV)' \
		NETGROUPNAME='$(NETGROUPNAME)' \
		NETUSERNAME='$(NETUSERNAME)' \
		PARAMSDIR='$(PARAMSDIR)' \
		PENDINGDIR='$(PENDINGDIR)' \
		QUEUE='$(QUEUE)' \
		REROUTEDIR='$(REROUTEDIR)' \
		RM='$(RM)' \
		ROUTINGDIR='$(ROUTINGDIR)' \
		SENDFILE='$(SENDFILE)' \
		SENDMAIL='$(SENDMAIL)' \
		SERVERGROUP='$(SERVERGROUP)' \
		SERVERUSER='$(SERVERUSER)' \
		SHELL='$(SHELL)' \
		SPOOLDIR='$(SPOOLDIR)' \
		STATEDIR='$(STATEDIR)' \
		STATEP='$(STATEP)' \
		STATERNOTLIST='$(STATERNOTLIST)' \
		STATSDIR='$(STATSDIR)' \
		STOP='$(STOP)' \
		STRIP='$(STRIP)' \
		SU='$(SU)' \
		SUN3='$(SUN3)' \
		SUN3LIBDIR='$(SUN3LIBDIR)' \
		SUN3SPOOLDIR='$(SUN3SPOOLDIR)' \
		SUN3USERNAME='$(SUN3USERNAME)' \
		TMPDIR='$(TMPDIR)' \
		TRACEDIR='$(TRACEDIR)' \
		WHOIS='$(WHOIS)' \
		WORKDIR='$(WORKDIR)'

certain all:	$(BIN) $(BIN)/strings-data.h
		@for dir in $(SOURCES); \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				echo $$dir ; \
				case $$dir in \
				    NNdaemon) \
					case x$(SUN3) in \
					x|x0)	touch Bin/NNdaemon Bin/NNlink ;; \
					*)	(cd $$dir; $(MAKE) $(MYMAKEDEFS)) || exit 1 ;; \
					esac ;; \
				    *)	(cd $$dir; $(MAKE) $(MYMAKEDEFS)) || exit 1 ;; \
				esac ; \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done

setup:
		@(cd VCsetup; $(MAKE) $(MYMAKEDEFS))

$(BIN):
		$(MKDIR) $(BIN)

$(BIN)/strings-data.h:	$(SITE_DEPEND) Lib/Version.c
		: making site dependencies
		@(cd Makefiles; $(MAKE) $(MYMAKEDEFS))
		@$(ECHO) "$(SITEFLAGS1) "|sed -e 's/[ 	][ 	]*/ /g' -e 's/$$/\\/' >site.out
		@$(ECHO) "$(SITEFLAGS2) "|sed -e 's/[ 	][ 	]*/ /g' -e 's/$$/\\/' >>site.out
		@$(ECHO) "$(SHELL) Makefiles/Site" >>site.out
		@$(SHELL) <site.out
		@grep LLONG $(INC)/site.h

directories $(INSBIN):
		@$(MAKE) -f Makefiles/Install $(IFLAGS) directories

special:	$(INSBIN)
		@$(MAKE) -f Makefiles/Install $(IFLAGS) special

install:	$(INSBIN)
		@$(MAKE) -f Makefiles/Install $(IFLAGS) install1
		@$(MAKE) -f Makefiles/Install $(IFLAGS) install2

manuals:	$(INSBIN)
		@cd Manuals; $(IFLAGS) $(SHELL) install$(MANTYPE)

config:		directories special install manuals
		@$(MAKE) -f Makefiles/Install $(IFLAGS) config

licence:
		@cd Licence; $(MAKE) $(MYMAKEDEFS) licence

log:
		@cd Admin; $(MAKE) $(MYMAKEDEFS) log

check test:
		@cd Admin; $(MAKE) $(MYMAKEDEFS) test

llib:		$(INC)/site.h $(BIN)/strings-data.h
		@-for dir in $(SOURCES); \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				echo $$dir ; \
				(cd $$dir ; $(MAKE) $(MYMAKEDEFS) llib.ln) ; \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done

lint:		$(INC)/site.h $(BIN)/strings-data.h
		@-for dir in Makefiles $(SOURCES) ; \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				(cd $$dir ; \
				echo $$dir ; \
				case $$dir in \
				Admin|Control|Handlers|HTdaemon|Init|NNdaemon|Utilities|VCdaemon|VCsetup) \
					$(MAKE) $(MYMAKEDEFS) CONFIG=-DNO_VOID_STAR "LINT=-lint -a -c" lint ;; \
				*) \
					$(MAKE) $(MYMAKEDEFS) CONFIG=-DNO_VOID_STAR "LINT=-lint -a -c -u" lint ;; \
				esac \
				) ; \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done

print:
		$(PRFORMATTER) README BUGS Makefile | $(PRSPOOL)
		@-for dir in Makefiles $(SOURCES); \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				echo $$dir ; \
				(cd $$dir ; \
				case x$(PRSPOOL) in \
				x$(PRSPOOLER)$(NAME)) \
					$(MAKE) $(MYMAKEDEFS) PRSPOOLER="$(PRSPOOLER)" PRFORMATTER="$(PRFORMATTER)" print ;; \
				x*) \
					$(MAKE) $(MYMAKEDEFS) PRSPOOL="$(PRSPOOL)" PRSPOOLER="$(PRSPOOLER)" PRFORMATTER="$(PRFORMATTER)" print ;; \
				esac \
				) ; \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done

clean:
		@-for dir in Makefiles $(SOURCES); \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				echo $$dir; \
				(cd $$dir; $(MAKE) $(MYMAKEDEFS) clean); \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done

clobber:
		@-for dir in Makefiles $(SOURCES); \
		do \
			if [ -f $$dir/Makefile ] ; \
			then \
				echo $$dir; \
				(cd $$dir; $(MAKE) $(MYMAKEDEFS) clobber); \
			else \
				: echo '*** ' $$dir not present ; \
			fi ; \
		done
		$(RM) $(BIN)/* $(INC)/site.h site.out made

FRC:
