#
#	SCCSID	@(#)Makefile	1.16 98/08/16
#

NAME		= VCsetup

IPLIBS		=

LIBS		= $(BINROUTE) $(BINLIB)
LLIBS		= 	      $(BINLIB)
SHELLLIBS	= $(BINROUTE) $(BINLIB)
YFLAGS		= -d

HEADERS		= call.h setup.h shell.h
CFILES		= CnnctSckt.c InitInterp.c MakeCall.c StringCRC.c call.c \
		  calldevfuns.c f_fd.c f_file.c f_udial.c f_test.c f_tty.c \
		  f_tcp.c f_udp.c f_x25.c f_xti.c \
		  globals.c interp.c lex.c match.c op.c pat.c symbol.c vlelt.c
OBJECTS		= CnnctSckt.o InitInterp.o MakeCall.o StringCRC.o call.o \
		  calldevfuns.o f_fd.o f_file.o f_udial.o f_test.o f_tty.o \
		  f_tcp.o f_udp.o f_x25.o f_xti.o \
		  globals.o interp.o lex.o match.o op.o pat.o symbol.o vlelt.o
LCFILES		= tcplisten.c
LOBJS		= tcplisten.o
SHELLFILES	= shell.c shelldevfuns.c a_tcp.c a_tty.c a_udp.c a_x25.c a_xti.c
SHELLOBJS	= shell.o shelldevfuns.o a_tcp.o a_tty.o a_udp.o a_x25.o a_xti.o StringCRC.o

BINARIES	= ../$(BIN)/VCcall ../$(BIN)/VCshell ../$(BIN)/tcplisten

all:		$(BINARIES)

../$(BIN)/VCshell: \
		$(SHELLOBJS) $(SHELLLIBS)
		$(CC) $(LDFLAGS) -o $@ $(SHELLOBJS) $(SHELLLIBS) $(IPLIBS) $(SLIBS) -lcrypt

../$(BIN)/VCcall: \
		$(OBJECTS) grammar.o $(LIBS)
		$(CC) $(LDFLAGS) -o $@ $(OBJECTS) grammar.o $(LIBS) $(IPLIBS) $(SLIBS)

../$(BIN)/tcplisten: \
		$(LOBJS) $(LLIBS)
		$(CC) $(LDFLAGS) -o $@ $(LOBJS) $(LLIBS) $(IPLIBS) $(SLIBS)

$(LOBJS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)
$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) call.h setup.h $(FRC)
$(SHELLOBJS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) shell.h setup.h $(FRC)

grammar.h:	grammar.y
		yacc -d grammar.y
		if [ -f y.tab.c ]; \
		then \
			mv y.tab.h grammar.h; \
			mv y.tab.c grammar.c; \
		fi

grammar.o:	grammar.h
		$(CC) $(CFLAGS) $(INCLUDE) $(DFLAGS) -c grammar.c

InitInterp.o interp.o lex.o symbol.o:	grammar.h

call.o:		$(INCArgs) $(INCaddress) $(INCexec) $(INChandlers) \
		$(INClink) $(INCparams) $(INCPasswd) $(INCroutefile) $(INCspool)
shell.o:	$(INCArgs) $(INCaddress) $(INCexec) \
		$(INClink) $(INCparams) $(INCPasswd) $(INCroutefile) $(INCspool)
tcplisten.o:	$(INCArgs) $(INCparams) $(INCspool)

lint:		call.lint shell.lint

call.lint:	call.t
		$(LIST) call.t

call.t:		setup.h call.h $(CFILES) grammar.h
		$(LINT) $(LFLAGS) $(CFILES) grammar.c \
		../$(LIB)/llib.ln \
		2>&1 | sed 's/^/	/' >$@

shell.lint:	shell.t
		$(LIST) shell.t

shell.t:	setup.h $(SHELLFILES)
		$(LINT) $(LFLAGS) $(SHELLFILES) \
		../$(ROUTE)/llib.ln ../$(LIB)/llib.ln \
		2>&1 | sed 's/^/	/' >$@

print:		Makefile $(HEADERS) $(CFILES) $(LCFILES) $(SHELLFILES)
		$(PRFORMATTER) Makefile $(HEADERS) $(CFILES) grammar.y $(SHELLFILES) $(LCFILES) | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) $(SHELLOBJS) $(LOBJS) grammar.[cho] *.t

clobber:	clean

llib.ln:

FRC:
