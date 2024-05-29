#
#	SCCSID	@(#)Makefile	1.6 92/09/23
#

NAME		= netinit

LIBS		= $(BINLIB)
YFLAGS		= -d
CFLAGS		= -g
LDFLAGS		= -g

HEADERS		= Netinit.h net.h
CFILES		= NetMessage.c Netinit.c Realloc.c RunProgs.c docron.c lex.c mkproc.c
OBJECTS		= NetMessage.o Netinit.o Realloc.o RunProgs.o docron.o lex.o mkproc.o
NETCFILES	= net.c
NETOBJECTS	= net.o

BINARIES	= ../$(BIN)/netinit ../$(BIN)/net

all:		$(BINARIES)

../$(BIN)/netinit: \
		$(OBJECTS) $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS) $(BINROUTE) $(BINLIB) $(SLIBS)

../$(BIN)/net: \
		$(NETOBJECTS) $(BINROUTE) $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(NETOBJECTS) $(BINROUTE) $(LIBS) $(SLIBS)

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) Netinit.h $(FRC)
$(NETOBJECTS):	$(INCglobal) $(INCdebug) $(INClink) $(INCPasswd) $(INCroutefile) $(INCsite) $(INCtypedefs) $(FRC)

Netinit.o net.o:	$(INCArgs) $(INCparams) $(INCspool)
Netinit.o NetMessage.o net.o:	net.h

lint:		Netinit.lint net.lint

Netinit.lint:	Netinit.t
		$(LIST) Netinit.t

Netinit.t:	$(HEADERS) $(CFILES)
		$(LINT) $(LFLAGS) $(CFILES) \
		../$(LIB)/llib.ln \
		2>&1 | sed 's/^/	/' >$@

net.lint:	net.t
		$(LIST) net.t

net.t:		net.h $(NETCFILES)
		$(LINT) $(LFLAGS) $(NETCFILES) \
		../$(LIB)/llib.ln \
		2>&1 | sed 's/^/	/' >$@

print:		Makefile $(HEADERS) $(CFILES) $(NETCFILES)
		$(PRFORMATTER) Makefile $(HEADERS) $(CFILES) $(NETCFILES) | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) $(NETOBJECTS) *.t

clobber:	clean

llib.ln:

FRC:
