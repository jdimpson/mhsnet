#
#	SCCSID	@(#)Makefile	1.7 93/04/28

NAME		= Route
LIBRARY		= $(BINROUTE)

CSFILES		= RouteStr.c
CSOBJS		= ../$(BIN)/RouteStr.o

CFILES		= Address.c CheckAddr.c DoRoute.c Legal_c.c PrintMaps.c PrintRoute.c ReadRoute.c UseRoute.c
OBJECTS		= Address.o CheckAddr.o DoRoute.o Legal_c.o PrintMaps.o PrintRoute.o ReadRoute.o UseRoute.o
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

../$(BIN)/RouteStr.o:	$(INCsite) $(INCstrings-data) RouteStr.c $(FRC)
		$(CC) $(CFLAGS) -I../$(BIN) $(DFLAGS) -c RouteStr.c
		$(MV) RouteStr.o ../$(BIN)

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)

Address.o:	$(INCaddress) $(INClink) $(INCroutefile)
CheckAddr.o:	$(INCaddress) $(INClink) $(INCroutefile) $(INCPasswd)
DoRoute.o:	$(INCaddress) $(INClink) $(INCroutefile)
PrintMaps.o:	$(INClink) $(INCroutefile)
PrintRoute.o:	$(INClink) $(INCroutefile)
ReadRoute.o:	$(INClink) $(INCroutefile)
UseRoute.o:	$(INCaddress) $(INClink) $(INCroutefile)

llib.ln:	llib-$(NAME) $(INCaddress) $(INCglobal) $(INClink) $(INCroutefile)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES) $(CSFILES) $(INCaddress) $(INClink) $(INCroutefile)
		$(LINT) $(LFLAGS) $(CFILES) $(CSFILES) ../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

print:		Makefile llib-$(NAME) $(HEADERS) $(CFILES) $(CSFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
