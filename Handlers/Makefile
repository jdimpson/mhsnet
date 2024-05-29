#
#	SCCSID	@(#)Makefile	1.8 92/09/23
#

NAME		= Handlers

BADLIBS		= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINROUTE) $(BINLIB)
FILERLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
HANDLERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
LOOPLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINROUTE) $(BINLIB)
MAILERLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
PETERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB)
PRINTERLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
ROUTERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
REPORTERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB) $(BINROUTE) $(BINLIB)
SERVERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
SPLRLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
STATERLIBS	= 		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
SUN34LIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
SUN43LIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
SUNSTLIBS	=		 $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)

HEADERS		=
CFILES		= bad.c filer.c fileserver.c handler.c loop.c mailer.c peter.c \
		  printer.c reporter.c router.c spooler.c stater.c Sun3_4.c Sun3stater.c Sun4_3.c
OBJECTS		= bad.o filer.o fileserver.o handler.o loop.o mailer.o peter.o \
		  printer.o reporter.o router.o spooler.o stater.o Sun3_4.o Sun3stater.o Sun4_3.o

all:		\
		../$(BIN)/badhandler \
		../$(BIN)/filer \
		../$(BIN)/fileserver \
		../$(BIN)/handler \
		../$(BIN)/loophandler \
		../$(BIN)/mailer \
		../$(BIN)/peter \
		../$(BIN)/printer \
		../$(BIN)/reporter \
		../$(BIN)/router \
		../$(BIN)/spooler \
		../$(BIN)/stater \
		../$(BIN)/Sun3_4 \
		../$(BIN)/Sun3stater \
		../$(BIN)/Sun4_3

../$(BIN)/badhandler:	\
		bad.o $(BADLIBS)
		$(CC) $(LDFLAGS) -o $@ bad.o $(BADLIBS) $(SLIBS)

../$(BIN)/filer:	\
		filer.o $(FILERLIBS)
		$(CC) $(LDFLAGS) -o $@ filer.o $(FILERLIBS) $(SLIBS)

../$(BIN)/fileserver:	\
		fileserver.o $(SERVERLIBS)
		$(CC) $(LDFLAGS) -o $@ fileserver.o $(SERVERLIBS) $(SLIBS)

../$(BIN)/handler:	\
		handler.o $(HANDLERLIBS)
		$(CC) $(LDFLAGS) -o $@ handler.o $(HANDLERLIBS) $(SLIBS)

../$(BIN)/loophandler:	\
		loop.o $(LOOPLIBS)
		$(CC) $(LDFLAGS) -o $@ loop.o $(LOOPLIBS) $(SLIBS)

../$(BIN)/mailer:	\
		mailer.o $(MAILERLIBS)
		$(CC) $(LDFLAGS) -o $@ mailer.o $(MAILERLIBS) $(SLIBS)

../$(BIN)/peter:	\
		peter.o $(PETERLIBS)
		$(CC) $(LDFLAGS) -o $@ peter.o $(PETERLIBS) $(SLIBS)

../$(BIN)/printer:	\
		printer.o $(PRINTERLIBS)
		$(CC) $(LDFLAGS) -o $@ printer.o $(PRINTERLIBS) $(SLIBS)

../$(BIN)/reporter:	\
		reporter.o $(REPORTERLIBS)
		$(CC) $(LDFLAGS) -o $@ reporter.o $(REPORTERLIBS) $(SLIBS)

../$(BIN)/router:	\
		router.o $(ROUTERLIBS)
		$(CC) $(LDFLAGS) -o $@ router.o $(ROUTERLIBS) $(SLIBS)

../$(BIN)/spooler:	\
		spooler.o $(SPLRLIBS)
		$(CC) $(LDFLAGS) -o $@ spooler.o $(SPLRLIBS) $(SLIBS)

../$(BIN)/stater:	\
		stater.o $(STATERLIBS)
		$(CC) $(LDFLAGS) -o $@ stater.o $(STATERLIBS) $(SLIBS)

../$(BIN)/Sun3_4:	\
		Sun3_4.o $(SUN34LIBS)
		$(CC) $(LDFLAGS) -o $@ Sun3_4.o $(SUN34LIBS) $(SLIBS)

../$(BIN)/Sun3stater:	\
		Sun3stater.o $(SUNSTLIBS)
		$(CC) $(LDFLAGS) -o $@ Sun3stater.o $(SUNSTLIBS) $(SLIBS)

../$(BIN)/Sun4_3:	\
		Sun4_3.o $(SUN43LIBS)
		$(CC) $(LDFLAGS) -o $@ Sun4_3.o $(SUN43LIBS) $(SLIBS)

$(OBJECTS):	$(INCArgs) $(INCcommandfile) $(INCexec) $(INCexpand) $(INCglobal) \
		$(INCheader) $(INCparams) $(INCsite) $(INCsysexits) $(INCtypedefs) $(FRC)

bad.o:		$(INCPasswd) $(INCspool) bad.c
filer.o:	$(INCaddress) $(INCforward) $(INCftheader) $(INCPasswd) $(INCspool) $(INCsub_proto) filer.c
fileserver.o:	$(INCaddress) $(INCforward) $(INCndir) $(INCPasswd) $(INCstats) $(INCspool) $(INCsub_proto) fileserver.c
handler.o:	$(INCaddress) $(INCforward) $(INCspool) handler.c
loop.o:		$(INChandlers) $(INCPasswd) loop.c
mailer.o:	$(INCaddress) $(INCforward) $(INCftheader) $(INCPasswd) $(INCspool) $(INCsub_proto) mailer.c
peter.o:	$(INCaddress) $(INCforward) $(INCPasswd) $(INCspool) $(INCsub_proto) peter.c
printer.o:	$(INCaddress) $(INCforward) $(INCftheader) $(INCPasswd) $(INCspool) $(INCsub_proto) printer.c
router.o:	$(INCaddress) $(INChandlers) $(INClicence) $(INClink) $(INCmsgsdb) $(INCPasswd) $(INCroutefile) $(INCspool) $(INCstatefile) $(INCstats) $(INCsub_proto) router.c
reporter.o:	$(INCforward) $(INCspool) $(INCsub_proto) reporter.c
spooler.o:	$(INCaddress) $(INCftheader) $(INChandlers) $(INCPasswd) $(INCspool) $(INCstatefile) $(INCstrings-data) $(INCsub_proto) spooler.c
stater.o:	$(INCaddress) $(INChandlers) $(INClink) $(INCPasswd) $(INCroute) $(INCroutefile) $(INCspool) $(INCstatefile) $(INCstrings-data) stater.c
Sun3_4.o:	$(INCaddress) $(INCftheader) $(INChandlers) $(INClink) $(INCPasswd) $(INCroutefile) $(INCspool) $(INCstatefile) $(INCstrings-data) $(INCsub_proto) Sun3_4.c
Sun3stater.o:	$(INCaddress) $(INClink) $(INCPasswd) $(INCroutefile) $(INCspool) $(INCstatefile) $(INCstrings-data) $(INCsub_proto) Sun3stater.c
Sun4_3.o:	$(INCaddress) $(INCftheader) $(INChandlers) $(INCPasswd) $(INCspool) $(INCstatefile) $(INCstrings-data) $(INCsub_proto) Sun4_3.c

lint:		\
		bad.lint \
		filer.lint \
		fileserver.lint \
		loop.lint \
		mailer.lint \
		printer.lint \
		reporter.lint \
		router.lint \
		spooler.lint \
		stater.lint \
		Sun3_4.lint \
		Sun3stater.lint \
		Sun4_3.lint

bad.lint:	bad.t
		$(LIST) bad.t

filer.lint:	filer.t
		$(LIST) filer.t

fileserver.lint:	fileserver.t
		$(LIST) fileserver.t

loop.lint:	loop.t
		$(LIST) loop.t

mailer.lint:	mailer.t
		$(LIST) mailer.t

peter.lint:	peter.t
		$(LIST) peter.t

printer.lint:	printer.t
		$(LIST) printer.t

reporter.lint:	reporter.t
		$(LIST) reporter.t

router.lint:	router.t
		$(LIST) router.t

spooler.lint:	spooler.t
		$(LIST) spooler.t

stater.lint:	stater.t
		$(LIST) stater.t

Sun3_4.lint:	Sun3_4.t
		$(LIST) Sun3_4.t

Sun3stater.lint:	Sun3stater.t
		$(LIST) Sun3stater.t

Sun4_3.lint:	Sun4_3.t
		$(LIST) Sun4_3.t

print:		Makefile handlers $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile handlers *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) *.t

clobber:	clean

llib.ln:

FRC:
