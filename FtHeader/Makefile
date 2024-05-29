#
#	SCCSID	@(#)Makefile	1.8 96/02/09
#

NAME		= FtHeader
LIBRARY		= $(BINFTHEADER)

CSFILES		= FtHeaderStr.c
CSOBJS		= ../$(BIN)/FtHeaderStr.o

CFILES		= AddrFromUser.c FreeFthFiles.c FtHeaderData.c FthToFree.c GetFthFiles.c \
		  GetFthTo.c InFthTo.c InitFtHead.c PrintFtHead.c ReadFtHead.c \
		  SetFthFiles.c SetFthTo.c WriteFtHead.c
OBJECTS		= AddrFromUser.o FreeFthFiles.o FtHeaderData.o FthToFree.o GetFthFiles.o \
		  GetFthTo.o InFthTo.o InitFtHead.o PrintFtHead.o ReadFtHead.o \
		  SetFthFiles.o SetFthTo.o WriteFtHead.o
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

../$(BIN)/FtHeaderStr.o:	$(INCsite) $(INCstrings-data) FtHeaderStr.c $(FRC)
		$(CC) $(CFLAGS) -I../$(BIN) $(DFLAGS) -c FtHeaderStr.c
		$(MV) FtHeaderStr.o ../$(BIN)

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCftheader) $(INCsite) $(INCtypedefs) $(FRC)

AddrFromUser.o:	$(INCaddress)
GetFthTo.o:	$(INCaddress)
InFthTo.o:	$(INCaddress)

llib.ln:	llib-$(NAME) $(INCaddress) $(INCftheader) $(INCglobal) $(INCheader)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out 
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES) $(CSFILES)
		$(LINT) $(LFLAGS) $(CFILES) $(CSFILES) \
		2>&1 | $(LSED) >$@

print:		Makefile llib-$(NAME) $(HEADERS) $(CFILES) $(CSFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
