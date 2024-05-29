#
#	SCCSID	@(#)Makefile	1.3 93/04/28
#

NAME		= Dirlib
LIBRARY		= $(BINDIRLIB)

HEADERS		=
CFILES		= closedir.c opendir.c readdir.c seekdir.c telldir.c
OBJECTS		= closedir.o opendir.o readdir.o seekdir.o telldir.o

$(LIBRARY):	$(OBJECTS)
		@-{ \
		set -x ; \
		if [ ! -f $@ ] ; \
		then \
			$(AR) $@ `$(LORDER) *.o | $(TSORT)` ; \
		else \
			$(AR) $@ $? ; \
		fi; \
		}
		$(RANLIB) $@

$(OBJECTS):	$(INCglobal) $(INCndir) $(INCsite) $(FRC)

lint:		lint.out
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES)
		$(LINT) $(LFLAGS) $(CFILES) \
		2>&1 | $(LSED) >$@

print:		$(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile *.h *.c | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

llib.ln:
		>llib.ln

FRC:
