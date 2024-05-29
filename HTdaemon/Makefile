#
#	SCCSID	@(#)Makefile	1.1 91/07/10
#

NAME		= HTdaemon

LIBS		= $(BINLIB) $(BINDIRLIB)

CFILES		= bufferio.c commands.c cook.c gap.c io.c \
		  main.c message.c packet.c \
		  reader.c receiver.c status.c tracepkt.c transmitter.c writer.c
HTOBJECTS	= bufferio.o commands.o cook.o gap.o io.o main.o message.o packet.o \
		  reader.o receiver.o status.o tracepkt.o transmitter.o writer.o
HEADERS		= channel.h chnstats.h dmnstats.h driver.h packet.h pktstats.h status.h

BINARIES	= ../$(BIN)/HTdaemon

all:		$(BINARIES)

../$(BIN)/HTdaemon: \
		$(HTOBJECTS) $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(HTOBJECTS) $(LIBS) $(SLIBS)

$(HTOBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(HEADERS) $(FRC)

commands.o:	$(INCcommandfile) $(INCexec) $(INCspool)
main.o:		$(INCArgs) $(INCparams) $(INCPasswd) $(INCspool)
message.o:	$(INCspool)
reader.o:	$(INCspool)
status.o:	$(INCspool)
writer.o:	$(INCspool)

lint:		HTdaemon.lint

HTdaemon.lint:	HTdaemon.t
		$(LIST) HTdaemon.t

HTdaemon.t:	$(HEADERS) $(CFILES)
		$(LINT) $(LFLAGS) $(CFILES) \
		../$(ROUTE)/llib.ln ../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile $(HEADERS) $(CFILES) | $(PRSPOOL)

clean:
		$(RM) $(LIBOBJECTS) $(PSOBJECTS) $(HTOBJECTS) linkstats.o *.t

clobber:	clean

llib.ln:

FRC:
