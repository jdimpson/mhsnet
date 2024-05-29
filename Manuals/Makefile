#
#	SCCSID	@(#)Makefile	1.35 95/07/28
#

NAME		= Manuals
MANGROUPNAME	= bin
MANUSERNAME	= bin

SPOOLDIR	= /usr/spool/MHSnet
ISPOOLDIR	= $(SPOOLDIR)
INSDIR		= $(ISPOOLDIR)/_man

COMPRESS	= 0
GZIP		= 0
PACK		= 0
STRIP		= 0
SUN3		= 1
USER		= 1

#	If `tail' isn't the standard `.[1-8]', then set the SECT? below.

COPY		= sed $(TRANSFORMS)
#COPY		= { sed $(TRANSFORMS) | nroff -Tlp -man; }
#COPY		= { ( cat tmac.an; sed $(TRANSFORMS) ) | nroff -Tlp; }
#COPY		= { sed $(TRANSFORMS) | compress; }
#COPY		= { sed $(TRANSFORMS) | nroff -Tlp -man | compress; }
CHGRP		= chgrp
CHOWN		= chown
CHMOD		= chmod

TRANSFORMS	= -e 's/^\.nr S3 [01]/.nr S3 $(SUN3)/' -e 's/[%]I[%]/1.3/' -e '/^\.TH/s/1\.[0-9][0-9]*/1.3/'

INSDIR1		= $(INSDIR)/man1
INSDIR3		= $(INSDIR)/man3
INSDIR5		= $(INSDIR)/man5
INSDIR7		= $(INSDIR)/man7
INSDIR8		= $(INSDIR)/man8

SECT1		= .1
SECT3		= .3
SECT5		= .5
SECT7		= .7
SECT8		= .8

#INSDIR1	= $(INSDIR)/man.MHS
#INSDIR3	= $(INSDIR)/man.MHS
#INSDIR5	= $(INSDIR)/man.MHS
#INSDIR7	= $(INSDIR)/man.MHS
#INSDIR8	= $(INSDIR)/man.MHS

#SECT1		= .MHS
#SECT3		= .MHS
#SECT5		= .MHS
#SECT7		= .MHS
#SECT8		= .MHS

DIRECTORIES	= \
		$(INSDIR) \
		$(INSDIR1) \
		$(INSDIR5) \
		$(INSDIR7) \
		$(INSDIR8)

INSTALLED	= \
		$(INSDIR1)/netcrc$(SECT1) \
		$(INSDIR1)/netdatenum$(SECT1) \
		$(INSDIR1)/netdis$(SECT1) \
		$(INSDIR1)/netfetch$(SECT1) \
		$(INSDIR1)/netfile$(SECT1) \
		$(INSDIR1)/netfinger$(SECT1) \
		$(INSDIR1)/netforward$(SECT1) \
		$(INSDIR1)/netftp$(SECT1) \
		$(INSDIR1)/netget$(SECT1) \
		$(INSDIR1)/netlink$(SECT1) \
		$(INSDIR1)/netmail$(SECT1) \
		$(INSDIR1)/netmap$(SECT1) \
		$(INSDIR1)/netprivs$(SECT1) \
		$(INSDIR1)/netq$(SECT1) \
		$(INSDIR1)/netrl$(SECT1) \
		$(INSDIR1)/netrq$(SECT1) \
		$(INSDIR1)/netstop$(SECT1) \
		$(INSDIR1)/nettimeout$(SECT1) \
		$(INSDIR1)/netwhois$(SECT1) \
		$(INSDIR5)/netfetch$(SECT5) \
		$(INSDIR5)/nethandlers$(SECT5) \
		$(INSDIR5)/netparams$(SECT5) \
		$(INSDIR5)/netpasswd$(SECT5) \
		$(INSDIR5)/netprivs$(SECT5) \
		$(INSDIR5)/netstats$(SECT5) \
		$(INSDIR7)/network$(SECT7) \
		$(INSDIR8)/netac$(SECT8) \
		$(INSDIR8)/netaddr$(SECT8) \
		$(INSDIR8)/netbad$(SECT8) \
		$(INSDIR8)/netcac$(SECT8) \
		$(INSDIR8)/netcall$(SECT8) \
		$(INSDIR8)/netcallers$(SECT8) \
		$(INSDIR8)/netchange$(SECT8) \
		$(INSDIR8)/netcheckdb$(SECT8) \
		$(INSDIR8)/netcheckq$(SECT8) \
		$(INSDIR8)/netcmds$(SECT8) \
		$(INSDIR8)/netconfig$(SECT8) \
		$(INSDIR8)/netcontrol$(SECT8) \
		$(INSDIR8)/netdaemon$(SECT8) \
		$(INSDIR8)/netfilter$(SECT8) \
		$(INSDIR8)/nethandler$(SECT8) \
		$(INSDIR8)/nethandlers$(SECT8) \
		$(INSDIR8)/nethdr$(SECT8) \
		$(INSDIR8)/netincorp$(SECT8) \
		$(INSDIR8)/netinit$(SECT8) \
		$(INSDIR8)/netlinkdata$(SECT8) \
		$(INSDIR8)/netlinklog$(SECT8) \
		$(INSDIR8)/netlisten$(SECT8) \
		$(INSDIR8)/netmsg$(SECT8) \
		$(INSDIR8)/netparams$(SECT8) \
		$(INSDIR8)/netpasswd$(SECT8) \
		$(INSDIR8)/netpath$(SECT8) \
		$(INSDIR8)/netpeeper$(SECT8) \
		$(INSDIR8)/netpurge$(SECT8) \
		$(INSDIR8)/netrequest$(SECT8) \
		$(INSDIR8)/netreroute$(SECT8) \
		$(INSDIR8)/netrouter$(SECT8) \
		$(INSDIR8)/netshell$(SECT8) \
		$(INSDIR8)/netspooler$(SECT8) \
		$(INSDIR8)/netsort$(SECT8) \
		$(INSDIR8)/netstart$(SECT8) \
		$(INSDIR8)/netstate$(SECT8) \
		$(INSDIR8)/netstatspr$(SECT8) \
		$(INSDIR8)/netstatus$(SECT8) \
		$(INSDIR8)/netwindup$(SECT8)

INSTALLED3	= \
		$(INSDIR3)/netargs$(SECT3)

all:

index:		# SCO only
		mkindex; cp index $(INSDIR)/man.MHS/index 
		$(CHMOD) u=rw,go=r $(INSDIR)/man.MHS/index

install:	$(INSTALLED)
		@-case "$(USER)" in \
		1)	echo "$(CHOWN) $(MANUSERNAME) ..."; \
			$(CHOWN) $(MANUSERNAME) $(INSTALLED) \
			;; \
		esac
		@-case "$(USER)" in \
		1)	echo "$(CHGRP) $(MANGROUPNAME) ..."; \
			$(CHGRP) $(MANGROUPNAME) $(INSTALLED) \
			;; \
		esac
		@echo "$(CHMOD) u=rw,go=r ..."
		@$(CHMOD) u=rw,go=r $(INSTALLED)
		@-case "$(STRIP)" in \
		1)	echo "strip trailing section numbers ..."; \
			for i in $(INSTALLED); \
			do \
				mv $$i `expr $$i : '\(.*\)\.[1-8]'`; \
			done \
			;; \
		esac
		@-case "$(PACK)" in \
		1)	echo "pack -f ..."; \
			pack -f $(INSTALLED) \
			;; \
		esac
		@-case "$(GZIP)" in \
		1)	echo "gzip -f ..."; \
			gzip -f $(INSTALLED) \
			;; \
		esac
		@-case "$(COMPRESS)" in \
		1)	echo "compress -f ..."; \
			compress -f $(INSTALLED) \
			;; \
		esac

install3:	$(INSTALLED3)
		@-case "$(USER)" in 1) set -x; $(CHOWN) $(MANUSERNAME) $(INSDIR3)/net*;; esac
		@-case "$(USER)" in 1) set -x; $(CHGRP) $(MANGROUPNAME) $(INSDIR3)/net*;; esac
		$(CHMOD) u=rw,go=r $(INSDIR3)/net*
		@-case "$(PACK)" in 1) set -x; pack -f $(INSDIR3)/*;; esac
		@-case "$(GZIP)" in 1) set -x; gzip -f $(INSDIR3)/*;; esac
		@-case "$(COMPRESS)" in 1) set -x; compress -f $(INSDIR3)/*;; esac

$(INSDIR1)/netcrc$(SECT1):	netcrc.1
		$(COPY) <netcrc.1 >$@

$(INSDIR1)/netdatenum$(SECT1):	netdatenum.1
		$(COPY) <netdatenum.1 >$@

$(INSDIR1)/netdis$(SECT1):	netdis.1
		$(COPY) <netdis.1 >$@

$(INSDIR1)/netfetch$(SECT1):	netfetch.1
		$(COPY) <netfetch.1 >$@

$(INSDIR1)/netfile$(SECT1):	netfile.1
		$(COPY) <netfile.1 >$@

$(INSDIR1)/netfinger$(SECT1):	netfinger.1
		$(COPY) <netfinger.1 >$@

$(INSDIR1)/netforward$(SECT1):	netforward.1
		$(COPY) <netforward.1 >$@

$(INSDIR1)/netftp$(SECT1):	netftp.1
		$(COPY) <netftp.1 >$@

$(INSDIR1)/netget$(SECT1):	netget.1
		$(COPY) <netget.1 >$@

$(INSDIR1)/netlink$(SECT1):	netlink.1
		$(COPY) <netlink.1 >$@

$(INSDIR1)/netmail$(SECT1):	netmail.1
		$(COPY) <netmail.1 >$@

$(INSDIR1)/netmap$(SECT1):	netmap.1
		$(COPY) <netmap.1 >$@

$(INSDIR1)/netprivs$(SECT1):	netprivs.1
		$(COPY) <netprivs.1 >$@

$(INSDIR1)/netq$(SECT1):	netq.1
		$(COPY) <netq.1 >$@

$(INSDIR1)/netrl$(SECT1):	netrl.1
		$(COPY) <netrl.1 >$@

$(INSDIR1)/netrq$(SECT1):	netrq.1
		$(COPY) <netrq.1 >$@

$(INSDIR1)/netstop$(SECT1):	netstop.1
		$(COPY) <netstop.1 >$@

$(INSDIR1)/nettimeout$(SECT1):	nettimeout.1
		$(COPY) <nettimeout.1 >$@

$(INSDIR1)/netwhois$(SECT1):	netwhois.1
		$(COPY) <netwhois.1 >$@

$(INSDIR3)/netargs$(SECT3):	netargs.3
		$(COPY) <netargs.3 >$@

$(INSDIR5)/netfetch$(SECT5):	netfetch.5
		$(COPY) <netfetch.5 >$@

$(INSDIR5)/nethandlers$(SECT5):	nethandlers.5
		$(COPY) <nethandlers.5 >$@

$(INSDIR5)/netparams$(SECT5):	netparams.5
		$(COPY) <netparams.5 >$@

$(INSDIR5)/netpasswd$(SECT5):	netpasswd.5
		$(COPY) <netpasswd.5 >$@

$(INSDIR5)/netprivs$(SECT5):	netprivs.5
		$(COPY) <netprivs.5 >$@

$(INSDIR5)/netstats$(SECT5):	netstats.5
		$(COPY) <netstats.5 >$@

$(INSDIR7)/network$(SECT7):	network.7
		$(COPY) <network.7 >$@

$(INSDIR8)/netac$(SECT8):	netac.8
		$(COPY) <netac.8 >$@

$(INSDIR8)/netaddr$(SECT8):	netaddr.8
		$(COPY) <netaddr.8 >$@

$(INSDIR8)/netbad$(SECT8):	netbad.8
		$(COPY) <netbad.8 >$@

$(INSDIR8)/netcac$(SECT8):	netcac.8
		$(COPY) <netcac.8 >$@

$(INSDIR8)/netcall$(SECT8):	netcall.8
		$(COPY) <netcall.8 >$@

$(INSDIR8)/netcallers$(SECT8):	netcallers.8
		$(COPY) <netcallers.8 >$@

$(INSDIR8)/netchange$(SECT8):	netchange.8
		$(COPY) <netchange.8 >$@

$(INSDIR8)/netcheckdb$(SECT8):	netcheckdb.8
		$(COPY) <netcheckdb.8 >$@

$(INSDIR8)/netcheckq$(SECT8):	netcheckq.8
		$(COPY) <netcheckq.8 >$@

$(INSDIR8)/netcmds$(SECT8):	netcmds.8
		$(COPY) <netcmds.8 >$@

$(INSDIR8)/netconfig$(SECT8):	netconfig.8
		$(COPY) <netconfig.8 >$@

$(INSDIR8)/netcontrol$(SECT8):	netcontrol.8
		$(COPY) <netcontrol.8 >$@

$(INSDIR8)/netdaemon$(SECT8):	netdaemon.8
		$(COPY) <netdaemon.8 >$@

$(INSDIR8)/netfilter$(SECT8):	netfilter.8
		$(COPY) <netfilter.8 >$@

$(INSDIR8)/nethandler$(SECT8):	nethandler.8
		$(COPY) <nethandler.8 >$@

$(INSDIR8)/nethandlers$(SECT8):	nethandlers.8
		$(COPY) <nethandlers.8 >$@

$(INSDIR8)/nethdr$(SECT8):	nethdr.8
		$(COPY) <nethdr.8 >$@

$(INSDIR8)/netincorp$(SECT8):	netincorp.8
		$(COPY) <netincorp.8 >$@

$(INSDIR8)/netinit$(SECT8):	netinit.8
		$(COPY) <netinit.8 >$@

$(INSDIR8)/netlinkdata$(SECT8):	netlinkdata.8
		$(COPY) <netlinkdata.8 >$@

$(INSDIR8)/netlinklog$(SECT8):	netlinklog.8
		$(COPY) <netlinklog.8 >$@

$(INSDIR8)/netlisten$(SECT8):	netlisten.8
		$(COPY) <netlisten.8 >$@

$(INSDIR8)/netmsg$(SECT8):	netmsg.8
		$(COPY) <netmsg.8 >$@

$(INSDIR8)/netparams$(SECT8):	netparams.8
		$(COPY) <netparams.8 >$@

$(INSDIR8)/netpasswd$(SECT8):	netpasswd.8
		$(COPY) <netpasswd.8 >$@

$(INSDIR8)/netpath$(SECT8):	netpath.8
		$(COPY) <netpath.8 >$@

$(INSDIR8)/netpeeper$(SECT8):	netpeeper.8
		$(COPY) <netpeeper.8 >$@

$(INSDIR8)/netpurge$(SECT8):	netpurge.8
		$(COPY) <netpurge.8 >$@

$(INSDIR8)/netrequest$(SECT8):	netrequest.8
		$(COPY) <netrequest.8 >$@

$(INSDIR8)/netreroute$(SECT8):	netreroute.8
		$(COPY) <netreroute.8 >$@

$(INSDIR8)/netrouter$(SECT8):	netrouter.8
		$(COPY) <netrouter.8 >$@

$(INSDIR8)/netshell$(SECT8):	netshell.8
		$(COPY) <netshell.8 >$@

$(INSDIR8)/netspooler$(SECT8):	netspooler.8
		$(COPY) <netspooler.8 >$@

$(INSDIR8)/netsort$(SECT8):	netsort.8
		$(COPY) <netsort.8 >$@

$(INSDIR8)/netstart$(SECT8):	netstart.8
		$(COPY) <netstart.8 >$@

$(INSDIR8)/netstate$(SECT8):	netstate.8
		$(COPY) <netstate.8 >$@

$(INSDIR8)/netstatspr$(SECT8):	netstatspr.8
		$(COPY) <netstatspr.8 >$@

$(INSDIR8)/netstatus$(SECT8):	netstatus.8
		$(COPY) <netstatus.8 >$@

$(INSDIR8)/netwindup$(SECT8):	netwindup.8
		$(COPY) <netwindup.8 >$@

directories:
		@set +e; \
		for dir in $(DIRECTORIES); \
		do \
			if [ ! -d $$dir ]; \
			then \
				mkdir $$dir || exit 1; \
				$(CHOWN) $(MANUSERNAME) $$dir; \
				$(CHGRP) $(MANGROUPNAME) $$dir; \
				$(CHMOD) ug=rwx,o=rx $$dir; \
			fi; \
			ls -ld $$dir; \
		done; \
		exit 0

print:

clean:

clobber:

llib.ln:

lint:

FRC:
