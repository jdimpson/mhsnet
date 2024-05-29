#
#	SCCSID	@(#)Makefile	1.3 94/03/03
#

NAME		= Config

CONFIG		= -DTERMCAP	# /etc/termcap present
TERMLIB		= -ltermlib
#CONFIG		=		# SYSTEM V
#TERMLIB	= -lcurses

BIN		= Bin

BINARIES	= ../$(BIN)/menu ../$(BIN)/answeryes

all:		$(BINARIES)

../$(BIN)/menu: \
		menu.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/menu menu.o $(TERMLIB) $(SLIBS)

../$(BIN)/answeryes: \
		answeryes.o $(BINLIB)
		$(CC) $(LDFLAGS) -o ../$(BIN)/answeryes answeryes.o $(SLIBS)

print:		Makefile $(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile *.h *.c | $(PRSPOOL)

cleanup:
		$(RM) *.lnk site.details site.name hostaddr initfile commandsfile

clean:		cleanup
		$(RM) *.o *.t

clobber:	clean

install:
		sh $(CONFIGDIR)/Install $(CONFIGDIR) $(SPOOLDIR)/$(CONFDIR) $(SPOOLDIR) $(NETUSERNAME) $(NETGROUPNAME)

llib.ln:

lint:

FRC:
