#
#	SCCSID	@(#)Makefile	1.7 05/08/26
#

NAME		= VCdaemon
LIBRARY		= $(BINVCLIB)

LIBS		=			   $(BINLIB) $(BINDIRLIB)
LINKLIBS	= ../Lib/SObuf.o $(BINROUTE) $(BINHEADER) $(LIBS)
VCSLIBS		= ../Lib/SObuf.o $(BINROUTE) $(BINHEADER) $(LIBS)

CFILES		= bufferio.c commands.c cook.c gap.c io.c \
		  main.c message.c packet.c \
		  reader.c receiver.c status.c tracepkt.c transmitter.c writer.c
LIBCFILES	= GetInFiles.c GetLnkState.c GetLnkTime.c
PSCFILES	= prtmain.c prtstats.c
LIBOBJECTS	= GetInFiles.o GetLnkState.o GetLnkTime.o
PSOBJECTS	= io.o prtmain.o prtstats.o
VCOBJECTS	= bufferio.o commands.o cook.o gap.o io.o main.o message.o packet.o \
		  reader.o receiver.o status.o tracepkt.o transmitter.o writer.o
HEADERS		= channel.h chnstats.h dmnstats.h driver.h packet.h pktstats.h status.h

BINARIES	= ../$(BIN)/linkstats ../$(BIN)/VCdaemon ../$(BIN)/VCstats

all:		$(LIBRARY) $(BINARIES)

$(LIBRARY):	$(LIBOBJECTS)
		@-{ \
		set -x ; \
		if [ ! -f $@ ] ; \
		then \
			$(AR) $@ `$(LORDER) $(LIBOBJECTS) | $(TSORT)` ; \
		else \
			$(AR) $@ $? ; \
		fi; \
		}
		$(RANLIB) $@

../$(BIN)/linkstats: \
		linkstats.o $(LINKLIBS)
		$(CC) $(LDFLAGS) -o $@ linkstats.o $(LINKLIBS) $(SLIBS)

../$(BIN)/VCdaemon: \
		$(VCOBJECTS) $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(VCOBJECTS) $(LIBS) $(SLIBS)

../$(BIN)/VCstats: \
		$(PSOBJECTS) $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(PSOBJECTS) $(VCSLIBS) $(SLIBS)

$(LIBOBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(HEADERS) $(FRC)
$(PSOBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(HEADERS) $(FRC)
$(VCOBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(HEADERS) $(FRC)
linkstats.o:	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(HEADERS) $(FRC)

commands.o:	$(INCcommandfile) $(INCexec) $(INCspool)
GetInFiles.o:	$(INCspool)
GetLnkState.o:	$(INCspool)
GetLnkTime.o:	$(INCspool)
linkstats.o linkstats.t:\
		$(INCaddress) $(INCArgs) $(INClink) $(INClockfile) $(INCroutefile) $(INCspool)
main.o:		$(INCArgs) $(INCparams) $(INCPasswd) $(INCspool)
message.o:	$(INCspool)
prtmain.o:	$(INCaddress) $(INCArgs) $(INClink) $(INCPasswd) $(INCroutefile) $(INCspool)
reader.o:	$(INCspool)
status.o:	$(INCspool)
writer.o:	$(INCspool)

lint:		linkstats.lint VCdaemon.lint VCstats.lint

linkstats.lint:	linkstats.t
		$(LIST) linkstats.t

VCdaemon.lint:	VCdaemon.t
		$(LIST) VCdaemon.t

VCdaemon.t:	$(HEADERS) $(CFILES)
		$(LINT) $(LFLAGS) $(CFILES) \
		../$(ROUTE)/llib.ln ../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

VCstats.lint:	VCstats.t
		$(LIST) VCstats.t

VCstats.t:	$(HEADERS) $(PSCFILES)
		$(LINT) $(LFLAGS) $(PSCFILES) \
		../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

print:		Makefile $(HEADERS) $(CFILES) $(PSCFILES) linkstats.c
		$(PRFORMATTER) Makefile $(HEADERS) $(CFILES) $(PSCFILES) linkstats.c | $(PRSPOOL)

clean:
		$(RM) $(LIBOBJECTS) $(PSOBJECTS) $(VCOBJECTS) linkstats.o *.t

clobber:	clean

llib.ln:

FRC:
