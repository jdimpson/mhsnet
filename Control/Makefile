#
#	SCCSID	@(#)Makefile	1.10 05/08/26
#

NAME		= Control

CHKMDB		= $(BINLIB) $(BINROUTE)
CHSTLIBS	= $(BINROUTE) $(BINHEADER) $(BINVCLIB) $(BINLIB) $(BINDIRLIB)
FLTRLIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB)
FLTR43LIBS	= $(BINFTHEADER) $(BINHEADER) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
MAPLIBS		= ../Lib/SObuf.o $(BINSTATE) $(BINROUTE) $(BINLIB)
PURGELIBS	= ../Lib/SObuf.o $(BINHEADER) $(BINROUTE) $(BINVCLIB) $(BINLIB) $(BINROUTE) $(BINLIB) $(BINDIRLIB)
REQLIBS		= $(BINHEADER) $(BINROUTE) $(BINLIB)
REROUTELIBS	= $(BINROUTE) $(BINLIB) $(BINDIRLIB)
STATELIBS	= ../Lib/SObuf.o $(BINSTATE) $(BINROUTE) $(BINLIB) $(BINROUTE)

CFILES		= changestate.c checkmsgsdb.c filter.c filter43.c map.c purge.c request.c reroute.c state.c
OBJECTS		= changestate.o checkmsgsdb.o filter.o filter43.o map.o purge.o request.o reroute.o state.o

BINARIES	= ../$(BIN)/changestate ../$(BIN)/checkmsgsdb ../$(BIN)/filter \
		  ../$(BIN)/filter43 ../$(BIN)/map ../$(BIN)/purge ../$(BIN)/request ../$(BIN)/reroute \
		  ../$(BIN)/state

all:		$(BINARIES)

../$(BIN)/changestate: \
		changestate.o $(CHSTLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/changestate changestate.o $(CHSTLIBS) $(SLIBS)

../$(BIN)/checkmsgsdb: \
		checkmsgsdb.o $(CHKMDB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/checkmsgsdb checkmsgsdb.o $(CHKMDB) $(SLIBS)

../$(BIN)/filter: \
		filter.o $(FLTRLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/filter filter.o $(FLTRLIBS) $(SLIBS)

../$(BIN)/filter43: \
		filter43.o $(FLTRLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/filter43 filter43.o $(FLTR43LIBS) $(SLIBS)

../$(BIN)/map: \
		map.o $(MAPLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/map map.o $(MAPLIBS) $(SLIBS)

../$(BIN)/purge: \
		purge.o $(PURGELIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/purge purge.o $(PURGELIBS) $(SLIBS)

../$(BIN)/request: \
		request.o $(REQLIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/request request.o $(REQLIBS) $(SLIBS)

../$(BIN)/reroute: \
		reroute.o $(REROUTELIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/reroute reroute.o $(REROUTELIBS) $(SLIBS)

../$(BIN)/state: \
		state.o $(STATELIBS)
		$(CC) $(LDFLAGS) -o ../$(BIN)/state state.o $(STATELIBS) $(SLIBS)

$(OBJECTS):	$(INCArgs) $(INCglobal) $(INCdebug) $(INCparams) $(INCsite) $(INCtypedefs) $(FRC)

changestate.o changestate.t:	\
		$(INCaddress) $(INCexec) $(INCexpand) $(INChandlers) $(INClink) \
		$(INCPasswd) $(INCroute) $(INCroutefile) $(INCspool) $(INCstrings-data) \
		changestate.c
checkmsgsdb.o checkmsgsdb.t:	\
		$(INCPasswd) $(INCmsgsdb) $(INCspool) \
		checkmsgsdb.c
filter.o filter.t:	\
		$(INCcommandfile) $(INCexec) $(INCftheader) $(INCheader) $(INCspool) $(INCsub_proto) \
		filter.c 
filter43.o filter43.t:	\
		$(INCaddress) $(INCcommandfile) $(INCftheader) $(INChandlers) \
		$(INCheader) $(INClink) $(INCroutefile) $(INCspool) $(INCstatefile) $(INCsub_proto) \
		filter43.c 
map.o map.t:	\
		$(INCaddress) $(INCexec) $(INClink) \
		$(INCroute) $(INCroutefile) $(INCstatefile) \
		map.c 
purge.o purge.t:	\
		$(INCcommandfile) $(INCPasswd) $(INCspool) \
		purge.c
request.o request.t:	\
		$(INCaddress) $(INCcommandfile) $(INCexec) \
		$(INChandlers) $(INCheader) $(INClink) $(INCPasswd) \
		$(INCroute) $(INCroutefile) $(INCspool) $(INCstatefile) \
		request.c 
reroute.o reroute.t:	\
		$(INCaddress) $(INCcommandfile) $(INCexec) \
		$(INClink) $(INCPasswd) $(INCroutefile) $(INCspool) \
		reroute.c 
state.o state.t:	\
		$(INCaddress) $(INCexec) $(INClink) $(INCPasswd) \
		$(INCroute) $(INCroutefile) $(INCstatefile) \
		state.c 

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) *.t

clobber:	clean

llib.ln:

lint:		changestate.lint checkmsgsdb.lint filter.lint filter43.lint map.lint purge.lint request.lint reroute.lint state.lint

changestate.lint:	changestate.t
		$(LIST) changestate.t

checkmsgsdb.lint:	checkmsgsdb.t
		$(LIST) checkmsgsdb.t

filter.lint:	filter.t
		$(LIST) filter.t

filter43.lint:	filter43.t
		$(LIST) filter43.t

map.lint:	map.t
		$(LIST) map.t

purge.lint:	purge.t
		$(LIST) purge.t

request.lint:	request.t
		$(LIST) request.t

reroute.lint:	reroute.t
		$(LIST) reroute.t

state.lint:	state.t
		$(LIST) state.t

FRC:
