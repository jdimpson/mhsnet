#
#	SCCSID	@(#)Makefile	1.7 04/07/26
#

NAME		= Header
LIBRARY		= $(BINHEADER)

CSFILES		= HeaderStr.c
CSOBJS		= ../$(BIN)/HeaderStr.o

CFILES		= AllParts.c DelEnv.c ExRevRoute.c ExRoute.c FreeHeader.c GetEnv.c GetEnvBool.c \
		  GetEnvNext.c HeaderData.c InRoute.c MakeEnv.c PrintHeader.c ReadHeader.c \
		  ReRoutable.c UpdateHeader.c WriteHeader.c
OBJECTS		= AllParts.o DelEnv.o ExRevRoute.o ExRoute.o FreeHeader.o GetEnv.o GetEnvBool.o \
		  GetEnvNext.o HeaderData.o InRoute.o MakeEnv.o PrintHeader.o ReadHeader.o \
		  ReRoutable.o UpdateHeader.o WriteHeader.o
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

../$(BIN)/HeaderStr.o:	$(INCsite) $(INCstrings-data) HeaderStr.c
		$(CC) $(CFLAGS) -I../$(BIN) $(DFLAGS) -c HeaderStr.c
		$(MV) HeaderStr.o ../$(BIN)

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCheader) $(INCsite) $(INCtypedefs) $(FRC)

AllParts.o:	$(INCcommandfile) $(INCspool)
InRoute.o:	$(INCaddress)
ReRoutable.o:	$(INCaddress)

llib.ln:	llib-$(NAME) $(INCaddress) $(INCglobal) $(INCheader)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out 
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES) $(CSFILES)
		$(LINT) $(LFLAGS) $(CFILES) $(CSFILES) ../$(LIB)/llib.ln \
		2>&1 | $(LSED) >$@

print:		Makefile llib-$(NAME) $(HEADERS) $(CFILES) $(CSFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
