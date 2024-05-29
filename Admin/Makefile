#
#	SCCSID	@(#)Makefile	1.40 05/12/16
#

NAME		= Admin

CONFIG		= -DTERMCAP	# /etc/termcap present
TERMLIB		= -ltermlib
#CONFIG		=		# SYSTEM V
#TERMLIB	= -lcurses

BIN		= Bin

ADDRLIBS	= ../Lib/SObuf.o $(BINROUTE) $(BINHEADER) $(BINLIB) $(BINDIRLIB)
CRCLIBS		= $(BINLIB)
CUTLIBS		= ../Lib/SObuf.o $(BINLIB)
DISLIBS		= ../Lib/SObuf.o $(BINLIB)
PWDLIBS		= $(BINROUTE) $(BINLIB)
REVPLIBS	= $(BINROUTE) $(BINLIB)
SHOWCMDLIBS	= $(BINHEADER) $(BINFTHEADER) $(BINLIB)
SHOWHNDLIBS	= ../Lib/SObuf.o $(BINLIB)
SHOWHDRLIBS	= $(BINROUTE) $(BINHEADER) $(BINFTHEADER) $(BINLIB)
SHOWPRMSLIBS	= ../Lib/SObuf.o $(BINROUTE) $(BINLIB)
SHOWPRVSLIBS	= ../Lib/SObuf.o $(BINLIB)
STATSLIBS	= ../Lib/SObuf.o $(BINHEADER) $(BINROUTE) $(BINLIB)

CFILES		= address.c crc.c cut.c dis.c passwd.c revpath.c showcmds.c showhandlers.c \
		  showhdr.c showparams.c showprivs.c statspr.c test-crc.c \
		  datenum.c days.c log.c numbers.c timeout.c
OBJECTS		= address.o crc.o cut.o dis.o passwd.o revpath.o showcmds.o showhandlers.o \
		  showhdr.o showparams.o showprivs.o statspr.o test-crc.o \
		  datenum.o days.o log.o numbers.o timeout.o

BINARIES	= ../$(BIN)/address ../$(BIN)/crc ../$(BIN)/cut ../$(BIN)/dis \
		  ../$(BIN)/revpath ../$(BIN)/passwd \
		  ../$(BIN)/showcmds ../$(BIN)/showhandlers ../$(BIN)/datenum \
		  ../$(BIN)/showhdr ../$(BIN)/showparams ../$(BIN)/showprivs \
		  ../$(BIN)/statspr ../$(BIN)/copydate ../$(BIN)/log ../$(BIN)/numbers \
		  ../$(BIN)/test-crc ../$(BIN)/timeout
OBINARIES	= ../$(BIN)/days ../$(BIN)/log

all:		../$(BIN)/curses.h $(BINARIES)

../$(BIN)/curses.h:
		if test -f /usr/include/curses.h; then \
			if grep NCURSES_ENABLE_STDBOOL_H /usr/include/curses.h >/dev/null; then \
				sed -e 's/NCURSES_ENABLE_STDBOOL_H.1/NCURSES_ENABLE_STDBOOL_H 0/g' \
					-e 's/__va_list/void */' \
					-e 's/\([ 	]\)move\( *(\)/\1_move\2/' \
					</usr/include/curses.h >$@ ; \
			else \
				sed -e 's/bool/_bool/g' -e 's/std_bool/stdbool/g' \
					-e 's/^#.*include.*<stdbool\.h>.*//' \
					-e 's/__va_list/void */' \
					-e 's/\([ 	]\)move\( *(\)/\1_move\2/' \
					</usr/include/curses.h >$@ ; \
			fi; \
			test ! -f /usr/include/termcap.h && ln $@ ../$(BIN)/termcap.h; \
		else \
			>$@; \
		fi; exit 0

../$(BIN)/address: \
		address.o $(ADDRLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/address address.o $(ADDRLIBS) $(SLIBS)

../$(BIN)/crc:	crc.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/crc crc.o $(CRCLIBS) $(SLIBS)

../$(BIN)/cut:	cut.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/cut cut.o $(CUTLIBS) $(SLIBS)

../$(BIN)/copydate:	copydate.sh
		cp copydate.sh ../$(BIN)/copydate
		chmod 755 ../$(BIN)/copydate

../$(BIN)/datenum:	datenum.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/datenum datenum.o $(SLIBS)

../$(BIN)/days:	days.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/days days.o $(BINLIB) $(SLIBS)

../$(BIN)/dis:	dis.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/dis dis.o $(DISLIBS) $(TERMLIB) $(SLIBS)

../$(BIN)/passwd: \
		passwd.o $(PWDLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/passwd passwd.o $(PWDLIBS) $(SLIBS) -lcrypt

../$(BIN)/revpath: \
		revpath.o $(REVPLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/revpath revpath.o $(REVPLIBS) $(SLIBS)

../$(BIN)/showcmds: \
		showcmds.o $(SHOWCMDLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/showcmds showcmds.o $(SHOWCMDLIBS) $(SLIBS)

../$(BIN)/showhandlers: \
		showhandlers.o $(SHOWHNDLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/showhandlers showhandlers.o $(SHOWHNDLIBS) $(SLIBS)

../$(BIN)/showhdr: \
		showhdr.o $(SHOWHDRLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/showhdr showhdr.o $(SHOWHDRLIBS) $(SLIBS)

../$(BIN)/showparams: \
		showparams.o $(SHOWPRMSLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/showparams showparams.o $(SHOWPRMSLIBS) $(SLIBS)

../$(BIN)/showprivs: \
		showprivs.o $(SHOWPRVSLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/showprivs showprivs.o $(SHOWPRVSLIBS) $(SLIBS)

../$(BIN)/statspr: \
		statspr.o $(STATSLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/statspr statspr.o $(STATSLIBS) $(SLIBS)

../$(BIN)/numbers:	numbers.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/numbers numbers.o $(BINLIB) $(SLIBS)

../$(BIN)/timeout:	timeout.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/timeout timeout.o $(BINLIB) $(SLIBS)

../$(BIN)/test-crc:	test-crc.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/test-crc test-crc.o $(BINLIB) $(SLIBS)

test:		../$(BIN)/test-crc
		../$(BIN)/test-crc

log ../$(BIN)/log:	log.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/log log.o $(BINLIB) $(SLIBS)

$(OBJECTS):	../$(BIN)/curses.h $(INCArgs) $(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)

address.o address.t:	\
		$(INCaddress) $(INCheader) $(INClink) $(INCPasswd) $(INCroutefile) $(INCspool) \
		address.c
passwd.o passwd.t:	\
		$(INCaddress) $(INCparams) $(INCPasswd) $(INCspool) \
		passwd.c
revpath.o revpath.t:	\
		$(INCaddress) $(INClink) $(INCroutefile) \
		revpath.c
showcmds.o showcmds.t:	\
		$(INCcommandfile) $(INCheader) \
		showcmds.c 
showhandlers.o showhandlers.t:	\
		$(INChandlers) $(INCspool) \
		showhandlers.c 
showhdr.o showhdr.t:	\
		$(INCaddress) $(INCcommandfile) $(INCheader) $(INClink) \
		$(INCftheader) $(INCPasswd) $(INCspool) $(INCsub_proto) \
		showhdr.c 
showparams.o showparams.t:	\
		$(INChandlers) $(INClicence) $(INClink) $(INCparams) $(INCPasswd) $(INCroutefile) $(INCspool) \
		showparams.c 
showprivs.o showprivs.t:	\
		$(INCPasswd) \
		showprivs.c 
statspr.o statspr.t:	\
		$(INCaddress) $(INChandlers) $(INCheader) $(INCparams) \
		$(INCstate) $(INCstats) \
		statspr.c 

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) *.t

clobber:	clean

llib.ln:

lint:		address.lint revpath.lint statspr.lint showcmds.lint showhdr.lint showparams.lint showprivs.lint test-crc.lint

address.lint:	address.t
		$(LIST) address.t

passwd.lint:	passwd.t
		$(LIST) passwd.t

revpath.lint:	revpath.t
		$(LIST) revpath.t

showcmds.lint:	showcmds.t
		$(LIST) showcmds.t

showhandlers.lint:	showhandlers.t
		$(LIST) showhandlers.t

showhdr.lint:	showhdr.t
		$(LIST) showhdr.t

showparams.lint:	showparams.t
		$(LIST) showparams.t

showprivs.lint:	showprivs.t
		$(LIST) showprivs.t

statspr.lint:	statspr.t
		$(LIST) statspr.t

test-crc.lint:	test-crc.t
		$(LIST) test-crc.t

FRC:
