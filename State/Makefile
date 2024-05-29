#
#	SCCSID	@(#)Makefile	1.8 93/04/28
#

NAME		= State
LIBRARY		= $(BINSTATE)

CSFILES		= StateStr.c
CSOBJS		= ../$(BIN)/StateStr.o

CFILES		= Check.c Comment.c Entry.c FreeRoute.c Link.c MakeRoute.c Malloc.c \
		  Paths.c Pcommand.c PrintAliases.c PrintFlags.c PrintForce.c PrintState.c \
		  R3state.c Rcommands.c RdStCommands.c ReadState.c Region.c Rstate.c SetPrint.c \
		  StateData.c Types.c WriteRoute.c WriteState.c
OBJECTS		= Check.o Comment.o Entry.o FreeRoute.o Link.o MakeRoute.o Malloc.o \
		  Paths.o Pcommand.o PrintAliases.o PrintFlags.o PrintForce.o PrintState.o \
		  R3state.o Rcommands.o RdStCommands.o ReadState.o Region.o Rstate.o SetPrint.o \
		  StateData.o Types.o WriteRoute.o WriteState.o
HEADERS		=

$(LIBRARY):	$(OBJECTS) $(CSOBJS)
		@-{ \
		set -x ; \
		if [ ! -f $@ ] ; \
		then \
			$(AR) $@ `$(LORDER) $(OBJECTS) $(CSOBJS) | $(TSORT)` ; \
		else \
			$(AR) $@ $? ; \
		fi; \
		}
		$(RANLIB) $@

../$(BIN)/StateStr.o:	$(INCsite) $(INCstrings-data) StateStr.c $(FRC)
		$(CC) $(CFLAGS) -I../$(BIN) $(DFLAGS) -c StateStr.c
		$(MV) StateStr.o ../$(BIN)

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INClink) $(INCroute) $(INCsite) $(INCtypedefs) $(FRC)

Check.o:	$(INCroutefile) $(INCstatefile)
Comment.o:	$(INCcommands) $(INCroutefile) $(INCstatefile)
Entry.o:	$(INCroutefile)
FreeRoute.o:	$(INCroutefile)
Link.o:		$(INCroutefile)
MakeRoute.o:	$(INCroutefile)
Paths.o:	$(INCroutefile)
Pcommand.o:	$(INCaddress) $(INCcommands) $(INCroutefile) $(INCstatefile)
PrintAliases.o:	$(INCroutefile)
PrintFlags.o:	$(INCcommands)
PrintState.o:	$(INCaddress) $(INCcommands) $(INCroutefile) $(INCstatefile)
RdStCommands.o:	$(INCstatefile)
Rcommands.o:	$(INCcommands) $(INCstatefile)
ReadState.o:	$(INCspool) $(INCstatefile)
Region.o:	$(INCaddress) $(INCstatefile)
R3state.o:	$(INCaddress) $(INCcommands) $(INCroutefile) $(INCstatefile) sun3state.h
Rstate.o:	$(INCcommands) $(INCroutefile) $(INCstatefile)
SetPrint.o:	$(INCroutefile) $(INCstatefile)
StateData.o:	$(INCstatefile)
Types.o:	$(INCaddress) $(INCcommands) $(INCroutefile) $(INCstatefile)
WriteRoute.o:	$(INCroutefile) $(INCspool)
WriteState.o:	$(INCroutefile) $(INCstatefile)

llib.ln:	llib-$(NAME) $(INCaddress) $(INCcommands) $(INCglobal) $(INClink) $(INCroute) $(INCroutefile) $(INCspool) $(INCstatefile)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES) $(CSFILES) $(INCaddress) $(INCcommands) $(INClink) $(INCroute) $(INCroutefile) $(INCstatefile)
		$(LINT) $(LFLAGS) $(CFILES) $(CSFILES) ../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

print:		Makefile llib-$(NAME) $(HEADERS) $(CFILES) $(CSFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
