#
#	SCCSID	@(#)Makefile	1.16 05/08/26
#

NAME		= NNdaemon

LIBS		=			   $(BINLIB) $(BINDIRLIB)
LINKLIBS	= ../Lib/SObuf.o $(BINROUTE) $(BINHEADER) $(LIBS)

CFILES		= Daemon/FindCommand.c Daemon/RCread.c Daemon/RCwrite.c Daemon/Rread.c Daemon/Rwrite.c \
		  Daemon/children.c Daemon/driver.c Daemon/in.c Daemon/main.c Daemon/out.c \
		  Daemon/queue.c Daemon/recvControl.c Daemon/sendControl.c Daemon/state.c Daemon/status.c
HEADERS		= Daemon/AQ.h Daemon/CQ.h Daemon/PQ.h Daemon/Stream.h Daemon/bad.h \
		  Daemon/daemon.h Daemon/driver.h Daemon/proto.h

PCFILES		= NNproto/Debug.c NNproto/Pinit.c NNproto/Pprintstats.c NNproto/Precv.c NNproto/Psend.c
PHEADERS	= NNproto/Channel.h NNproto/Debug.h NNproto/Packet.h NNproto/Pconfig.h NNproto/Proto.h \
		  NNproto/Pstats.h

OBJS		= Daemon/FindCommand.o \
		  ../VCdaemon/cook.o \
		  Daemon/children.o Daemon/in.o Daemon/io.o Daemon/main.o Daemon/out.o \
		  Daemon/recvControl.o Daemon/sendControl.o Daemon/state.o Daemon/status.o
NNOBJS		= NNproto/RCread.o NNproto/RCwrite.o NNproto/Rread.o NNproto/Rwrite.o \
		  NNproto/driver.o NNproto/queue.o \
		  NNproto/Debug.o NNproto/Pinit.o NNproto/Pprintstats.o NNproto/Precv.o NNproto/Psend.o
CNOBJS		= CNproto/RCread.o CNproto/RCwrite.o CNproto/Rread.o CNproto/Rwrite.o \
		  CNproto/driver.o CNproto/queue.o \
		  CNproto/Debug.o CNproto/Pinit.o CNproto/Pprintstats.o CNproto/Precv.o CNproto/Psend.o
ENOBJS		= ENproto/RCread.o ENproto/RCwrite.o ENproto/Rread.o ENproto/Rwrite.o \
		  ENproto/driver.o ENproto/queue.o \
		  ENproto/Debug.o ENproto/Pinit.o ENproto/Pprintstats.o ENproto/Precv.o ENproto/Psend.o
PNOBJS		= PNproto/RCread.o PNproto/RCwrite.o PNproto/Rread.o PNproto/Rwrite.o \
		  PNproto/driver.o PNproto/queue.o \
		  PNproto/Debug.o PNproto/Pinit.o PNproto/Pprintstats.o PNproto/Precv.o PNproto/Psend.o

INCAQ		= Daemon/AQ.h
INCCQ		= Daemon/CQ.h
INCbad		= Daemon/bad.h
INCdaemon	= Daemon/daemon.h
INCdriver	= Daemon/driver.h
INCStream	= Daemon/Stream.h
INCproto	= Daemon/proto.h
INCPconfig	= NNproto/Pconfig.h
IN		= -IDaemon -INNproto -I..

MYMAKEDEFS	= \
		-f ../Makefiles/Makefile0 -f MakefileP \
		AR='$(AR)' \
		AS='$(AS)' \
		BIN='$(BIN)' \
		CC='$(CC)' \
		CFLAGS='$(CFLAGS)' \
		CONFIG='$(CONFIG)' \
		DEBUG='$(DEBUG)' \
		DIRLIB='$(DIRLIB)' \
		FRC='$(FRC)' \
		FTHEADER='$(FTHEADER)' \
		HEADER='$(HEADER)' \
		INC='$(INC)' \
		LDFLAGS='$(LDFLAGS)' \
		LIB='$(LIB)' \
		LIBC='$(LIBC)' \
		LN='$(LN)' \
		LORDER='$(LORDER)' \
		MAKE='$(MAKE)' \
		MKDIR='$(MKDIR)' \
		MV='$(MV)' \
		RANLIB='$(RANLIB)' \
		RM='$(RM)' \
		ROUTE='$(ROUTE)' \
		SHELL='$(SHELL)' \
		STATE='$(STATE)' \
		SLIBS='$(SLIBS)' \
		TSORT='$(TSORT)'

all:		../$(BIN)/NNdaemon ../$(BIN)/CNdaemon ../$(BIN)/ENdaemon ../$(BIN)/PNdaemon ../$(BIN)/NNlink

pobjs:
		@$(MAKE) $(MYMAKEDEFS) PROTO=NNproto PROTODEF=NNstrpr
		@$(MAKE) $(MYMAKEDEFS) PROTO=CNproto PROTODEF=NNstrpr2
		@$(MAKE) $(MYMAKEDEFS) PROTO=ENproto PROTODEF=ENproto
		@$(MAKE) $(MYMAKEDEFS) PROTO=PNproto PROTODEF=PNproto

../$(BIN)/NNdaemon: \
		$(OBJS) pobjs $(LIBS)
		@case '$?' in ''|pobjs) ;; *) set -x; \
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(NNOBJS) $(LIBS) $(SLIBS) ;; esac

../$(BIN)/CNdaemon: \
		$(OBJS) pobjs $(LIBS)
		@case '$?' in ''|pobjs) ;; *) set -x; \
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(CNOBJS) $(LIBS) $(SLIBS) ;; esac

../$(BIN)/ENdaemon: \
		$(OBJS) pobjs $(LIBS)
		@case '$?' in ''|pobjs) ;; *) set -x; \
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(ENOBJS) $(LIBS) $(SLIBS) ;; esac

../$(BIN)/PNdaemon: \
		$(OBJS) pobjs $(LIBS)
		@case '$?' in ''|pobjs) ;; *) set -x; \
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(PNOBJS) $(LIBS) $(SLIBS) ;; esac

../$(BIN)/NNlink: \
		linkstats.o $(LINKLIBS)
		$(CC) $(LDFLAGS) -o $@ linkstats.o $(LINKLIBS) $(SLIBS)

$(OBJS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)
linkstats.o:	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)

Daemon/FindCommand.o: \
		$(INCAQ) $(INCStream) $(INCbad) $(INCdaemon) $(INCdriver) $(INCspool) \
		 Daemon/FindCommand.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/FindCommand.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/FindCommand.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/children.o: \
		$(INCAQ) $(INCStream) $(INCbad) $(INCcommandfile) $(INCdaemon) $(INCdriver) $(INCspool) \
		 Daemon/children.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/children.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/children.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/in.o: \
		$(INCAQ) $(INCdaemon) $(INCdriver) $(INCStream) $(INCspool) \
		 Daemon/in.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/in.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/in.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/io.o: \
		$(INCdaemon) $(INCdriver) \
		 Daemon/io.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/io.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/io.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/main.o: \
		 ../VCdaemon/cook.h \
		$(INCArgs) $(INCdaemon) $(INCdriver) $(INCStream) $(INCspool) \
		 Daemon/main.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/main.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/main.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/out.o: \
		$(INCAQ) $(INCCQ) $(INCStream) $(INCcommandfile) $(INCbad) $(INCdaemon) \
		 $(INCdriver) $(INCspool) \
		 Daemon/out.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/out.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/out.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/recvControl.o: \
		$(INCAQ) $(INCCQ) $(INCdaemon) $(INCdriver) $(INCStream) $(INCproto) \
		 Daemon/recvControl.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/recvControl.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/recvControl.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/sendControl.o: \
		$(INCdriver) $(INCStream) $(INCproto) \
		 Daemon/sendControl.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/sendControl.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/sendControl.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/state.o: \
		$(INCAQ) $(INCPconfig) $(INCStream) $(INCdaemon) $(INCdriver) \
		 Daemon/state.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/state.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c Daemon/state.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
Daemon/status.o: \
		$(INCdaemon) \
		 ../VCdaemon/chnstats.h ../VCdaemon/channel.h ../VCdaemon/dmnstats.h \
		 ../VCdaemon/pktstats.h ../VCdaemon/status.h \
		 ../VCdaemon/cook.h \
		 Daemon/status.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c Daemon/status.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) $(DFLAGS) -c Daemon/status.c -o $@
		@-f=`basename $@`; test -f $$f && mv $$f $@
linkstats.o linkstats.t:\
		$(INCaddress) $(INCArgs) $(INClink) $(INClockfile) $(INCroutefile) $(INCspool) $(INCStream) \
		 linkstats.c
		@echo $(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr -c linkstats.c; \
		$(CC) $(CFLAGS) $(IN) $(INCLUDE) -DNNstrpr $(DFLAGS) -c linkstats.c

lint:		linkstats.lint NNdaemon.lint

NNdaemon.lint:	NNdaemon.t
		$(LIST) NNdaemon.t

linkstats.lint:	linkstats.t
		$(LIST) linkstats.t

NNdaemon.t:	$(HEADERS) $(CFILES) $(PHEADERS) $(PCFILES)
		$(LINT) $(IN) $(LFLAGS) $(CFILES) $(PCFILES) -DNNstrpr \
		../$(LIB)/llib.ln \
		2>&1 | sed 's/^/	/' >$@

print:		Makefile $(HEADERS) $(CFILES) $(PHEADERS) $(PCFILES)
		$(PRFORMATTER) Makefile* $(HEADERS) $(CFILES) $(PHEADERS) $(PCFILES) | $(PRSPOOL)

clean:
		$(RM) $(OBJS) $(NNOBJS) $(CNOBJS) $(ENOBJS) $(PNOBJS) linkstats.o *.t

clobber:	clean

llib.ln:

FRC:
