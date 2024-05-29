#
#	SCCSID	@(#)Makefile	1.8 95/07/28
#

NAME		= CLibrary
LIBRARY		= $(BINLIBC)

CFILES		= malloc_debug.c regcmp.c regex.c regexp.c regsub.c strchr.c \
		  strerror.c strpbrk.c strrchr.c strspn.c strtok.c uucplock.c vfprintf.c
OBJECTS		= malloc_debug.o regcmp.o regex.o regexp.o regsub.o strchr.o \
		  strerror.o strpbrk.o strrchr.o strspn.o strtok.o uucplock.o vfprintf.o
HEADERS		=


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

$(OBJECTS):	$(INCglobal) $(INCsite) $(INCtypedefs) $(FRC)

llib.ln:	llib-$(NAME)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out llib-$(NAME)
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES)
		$(LINT) $(LFLAGS) $(CFILES) \
		2>&1 | $(LSED) >$@

print:		$(HEADERS) $(CFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) $(HEADERS) $(CFILES) | $(PRSPOOL)

clean:
		$(RM) $(OBJECTS) lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
