#
#	SCCSID	@(#)Makefile	1.6 05/08/26
#

NAME		= Utilities

FETCHLIBS	=					 	      $(BINLIB)
FORWDLIBS	= ../Lib/SObuf.o $(BINFTHEADER)	      $(BINROUTE)	      $(BINLIB) $(BINDIRLIB)
GETFLIBS	= ../Lib/SObuf.o $(BINFTHEADER) $(BINHEADER) $(BINROUTE)	      $(BINLIB) $(BINDIRLIB)
QUEUELIBS	= ../Lib/SObuf.o $(BINFTHEADER) $(BINROUTE) $(BINHEADER) $(BINVCLIB) $(BINLIB) $(BINDIRLIB)
SENDFLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE)	      $(BINLIB)
SENDGLIBS	=		 $(BINHEADER) $(BINROUTE)	      $(BINLIB)
SENDMLIBS	=			      $(BINROUTE)	      $(BINLIB)
WHOISLIBS	=						      $(BINLIB)

HEADERS		=
CFILES		= fetchfile.c forward.c getfile.c queue.c sendfile.c sendmail.c sendmsg.c whois.c
OBJECTS		= fetchfile.o forward.o getfile.o queue.o sendfile.o sendmail.o sendmsg.o whois.o

BINARIES	= ../$(BIN)/fetchfile ../$(BIN)/forward ../$(BIN)/getfile ../$(BIN)/queue ../$(BIN)/sendfile ../$(BIN)/sendmail ../$(BIN)/sendmsg ../$(BIN)/whois

all:		$(BINARIES)

../$(BIN)/fetchfile:	\
		fetchfile.o $(FETCHLIBS)
		$(CC) $(LDFLAGS) -o $@ fetchfile.o $(FETCHLIBS) $(SLIBS)

../$(BIN)/forward:	\
		forward.o $(FORWDLIBS)
		$(CC) $(LDFLAGS) -o $@ forward.o $(FORWDLIBS) $(SLIBS)

../$(BIN)/getfile:	\
		getfile.o $(GETFLIBS)
		$(CC) $(LDFLAGS) -o $@ getfile.o $(GETFLIBS) $(SLIBS)

../$(BIN)/queue:	\
		queue.o $(QUEUELIBS)
		$(CC) $(LDFLAGS) -o $@ queue.o $(QUEUELIBS) $(SLIBS)

../$(BIN)/sendfile:	\
		sendfile.o $(SENDFLIBS)
		$(CC) $(LDFLAGS) -o $@ sendfile.o $(SENDFLIBS) $(SLIBS)

../$(BIN)/sendmail: \
		sendmail.o $(SENDMLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/sendmail sendmail.o $(SENDMLIBS) $(SLIBS)

../$(BIN)/sendmsg:	\
		sendmsg.o $(SENDGLIBS)
		$(CC) $(LDFLAGS) -o $@ sendmsg.o $(SENDGLIBS) $(SLIBS)

../$(BIN)/whois:	\
		whois.o $(WHOISLIBS)
		$(CC) $(LDFLAGS) -o $@ whois.o $(WHOISLIBS) $(SLIBS)

$(OBJECTS):	$(INCaddress) $(INCArgs) $(INCcommandfile) $(INCglobal) $(INCheader) $(INCPasswd) $(INCsite) $(INCspool) $(INCtypedefs) $(FRC)

fetchfile.o fetchfile.t:$(INCexec)     $(INChandlers)						 fetchfile.c
forward.o forward.t:	$(INCftheader) $(INChandlers) $(INClink) $(INCroutefile) $(INCsub_proto) forward.c
getfile.o getfile.t:	$(INCftheader) $(INCexec)     $(INClink) $(INCroutefile)		 getfile.c
queue.o queue.t:	$(INCftheader) $(INChandlers) $(INClink) $(INCroutefile) $(INCsub_proto) queue.c
sendfile.o sendfile.t:	$(INCftheader) $(INChandlers) $(INClink) $(INCroutefile) $(INCsub_proto) sendfile.c
sendmail.o sendmail.t:  $(INCexec)		      $(INClink) $(INCroutefile)		 sendmail.c
sendmsg.o sendmsg.t:		       $(INChandlers) $(INClink) $(INCroutefile) $(INCsub_proto) sendmsg.c
whois.o whois.t:	$(INCexec)     $(INChandlers)						 whois.c

queue.o queue.t:	$(INClockfile)

lint:		fetchfile.lint forward.lint getfile.lint queue.lint sendfile.lint sendmail.lint sendmsg.lint whois.lint

fetchfile.lint:	fetchfile.t
		$(LIST) fetchfile.t

forward.lint:	forward.t
		$(LIST) forward.t

getfile.lint:	getfile.t
		$(LIST) getfile.t

queue.lint:	queue.t
		$(LIST) queue.t

sendfile.lint:	sendfile.t
		$(LIST) sendfile.t

sendmail.lint:	sendmail.t
		$(LIST) sendmail.t

sendmsg.lint:	sendmsg.t
		$(LIST) sendmsg.t

whois.lint:	whois.t
		$(LIST) whois.t

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile $(HEADERS) $(CFILES) | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) *.t

clobber:	clean

llib.ln:

FRC:
