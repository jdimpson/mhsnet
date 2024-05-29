#
#	SCCSID	@(#)Makefile	1.6 94/04/03
#

NAME		= Makefiles

CFILES		= bit-test.c pipe-test.c sprf-test.c
OBJECTS		= bit-test.o pipe-test.o sprf-test.o

BINARIES	= ../$(BIN)/bit-test ../$(BIN)/pipe-test ../$(BIN)/sprf-test

all:		$(BINARIES)

../$(BIN)/bit-test:	bit-test.o
		$(CC) $(LDFLAGS) -o ../$(BIN)/bit-test bit-test.o $(SYSLIBS)

../$(BIN)/pipe-test:	pipe-test.o
		$(CC) $(LDFLAGS) -o ../$(BIN)/pipe-test pipe-test.o $(SYSLIBS)

../$(BIN)/sprf-test:	sprf-test.o
		$(CC) $(LDFLAGS) -o ../$(BIN)/sprf-test sprf-test.o $(SYSLIBS)

test:		all
		../$(BIN)/bit-test
		../$(BIN)/sprf-test
		../$(BIN)/pipe-test

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile $(CFILES) Makefile0 Site Install | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) *.t

clobber:	clean
		$(RM) $(BINARIES)

llib:

lint:		bit-test.lint pipe-test.lint sprf-test.lint

bit-test.lint:	bit-test.t
		$(LIST) bit-test.t

pipe-test.lint:	pipe-test.t
		$(LIST) pipe-test.t

sprf-test.lint:	sprf-test.t
		$(LIST) sprf-test.t

FRC:
