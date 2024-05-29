#
#	SCCSID	@(#)Makefile	1.28 07/01/03
#

NAME		= Library
LIBRARY		= $(BINLIB)
SHELL		= /bin/sh

CSFILES		= LibStr.c ServerStr.c
CSOBJS		= ../$(BIN)/LibStr.o ../$(BIN)/ServerStr.o

CFILES		= \
		AddCmd.c AddressMatch.c Args.c ArgsUsage.c CanAddrMatch.c \
		CanReadPipe.c ChangeCmd.c CheckData.c CheckDirs.c CleanError.c \
		CmdToString.c CmdsData.c CollectData.c ConvSun3Cmds.c CopyCmds.c \
		CopyFileToFd.c CpuStats.c DaemonActive.c DateString.c DecodeNum.c \
		Detach.c DoArgs.c DomainToPath.c EchoArgs.c EncodeNum.c Execute.c \
		ExpandArgs.c ExpandFile.c ExpandString.c Fatal.c Forwarded.c FreeCmds.c \
		FreeFileCmds.c FSFree.c FreeStr.c GetErrFile.c GetForward.c GetHandler.c \
		GetInvoker.c GetNetUid.c GetParams.c GetPassword.c GetPrivs.c GetUser.c \
		InList.c LinkCmds.c Locking.c MailNCC.c MakeErrFile.c MakeLicence.c \
		MakeLock.c Malloc.c MesgN.c MkDirs.c MoveCmds.c MsgsDB.c \
		NameProg.c NodeName.c NumericArg.c QuoteChars.c ReRoute.c ReadCmds.c \
		ReadFd.c ReadFdCmds.c ReadErrFile.c ReadFile.c RmDir.c SMdate.c SObuf.c SetForward.c \
		SetLock.c SetRaw.c SetUlimit.c SetUser.c Shell.c SplitArg.c StderrLog.c \
		StringCRC32.c StringMatch.c StripCopyEnd.c StripDomain.c StripEnv.c StripTypes.c \
		SysWarn.c TimeString.c ToLower.c ToUpper.c Trace.c UniqueName.c ValidChars.c Version.c \
		Warn.c WrStats.c WriteCmds.c bcopy.c bsearch.c concat.c crc.c crc32.c create.c \
		lfind.c listsort.c millisecs.c newprintf.c newnstr.c newstr.c otol.c \
		rfc822time.c strccmp.c strclr.c strcpyend.c strnccmp.c \
		strncpyend.c strrpbrk.c ticks.c xtol.c

OBJECTS		= $(CRCS_O) \
		AddCmd.o AddressMatch.o Args.o ArgsUsage.o CanAddrMatch.o \
		CanReadPipe.o ChangeCmd.o CheckData.o CheckDirs.o CleanError.o \
		CmdToString.o CmdsData.o CollectData.o ConvSun3Cmds.o CopyCmds.o \
		CopyFileToFd.o CpuStats.o DaemonActive.o DateString.o DecodeNum.o \
		Detach.o DoArgs.o DomainToPath.o EchoArgs.o EncodeNum.o Execute.o \
		ExpandArgs.o ExpandFile.o ExpandString.o Fatal.o Forwarded.o FreeCmds.o \
		FreeFileCmds.o FSFree.o FreeStr.o GetErrFile.o GetForward.o GetHandler.o \
		GetInvoker.o GetNetUid.o GetParams.o GetPassword.o GetPrivs.o GetUser.o \
		InList.o LinkCmds.o Locking.o MailNCC.o MakeErrFile.o MakeLicence.o \
		MakeLock.o Malloc.o MesgN.o MkDirs.o MoveCmds.o MsgsDB.o \
		NameProg.o NodeName.o NumericArg.o QuoteChars.o ReRoute.o ReadCmds.o \
		ReadFd.o ReadFdCmds.o ReadErrFile.o ReadFile.o RmDir.o SMdate.o SObuf.o SetForward.o \
		SetLock.o SetRaw.o SetUlimit.o SetUser.o Shell.o SplitArg.o StderrLog.o \
		StringCRC32.o StringMatch.o StripCopyEnd.o StripDomain.o StripEnv.o StripTypes.o \
		SysWarn.o TimeString.o ToLower.o ToUpper.o Trace.o UniqueName.o ValidChars.o Version.o \
		Warn.o WrStats.o WriteCmds.o bcopy.o bsearch.o concat.o crc.o crc32.o create.o \
		lfind.o listsort.o millisecs.o newprintf.o newnstr.o newstr.o otol.o \
		rfc822time.o strccmp.o strclr.o strcpyend.o strnccmp.o \
		strncpyend.o strrpbrk.o ticks.o xtol.o

LFILES		= \
		Error.c Report.c MesgV.c DateTimeStr.c
LOBJS		= \
		Error.o Report.o MesgV.o DateTimeStr.o
HEADERS		=

$(LIBRARY):	$(OBJECTS) $(CSOBJS) $(LOBJS)
		@echo "$(LORDER) \\" >ar.tmp
		@echo "$(OBJECTS) \\" >>ar.tmp
		@echo "$(CSOBJS) | $(TSORT)" >>ar.tmp; \
		set -x ; \
		if [ ! -f $@ ] ; \
		then \
			$(AR) $@ `$(SHELL) <ar.tmp` $(LOBJS) ; \
		else \
			$(AR) $@ $? ; \
		fi ; \
		$(RM) ar.tmp ; \
		$(RANLIB) $@

$(CSOBJS):	$(INCsite) $(INCstrings-data) $(INCtypedefs) $(FRC)

../$(BIN)/LibStr.o:	LibStr.c
		$(CC) $(CFLAGS) -I../$(BIN) -I../$(INC) $(DFLAGS) -c LibStr.c
		$(MV) LibStr.o ../$(BIN)

../$(BIN)/ServerStr.o:	ServerStr.c
		$(CC) $(CFLAGS) -I../$(BIN) -I../$(INC) $(DFLAGS) -c ServerStr.c
		$(MV) ServerStr.o ../$(BIN)

crcs.s:		crc.c $(INCglobal) $(INCsite) $(FRC)
		-set -x ; \
		$(CC) -DAS -E $(INCLUDE) $(DFLAGS) crc.c \
		| egrep -v '^#(line)* [1-9]' >$@

$(OBJECTS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)
$(LOBJS):	$(INCglobal) $(INCdebug) $(INCsite) $(INCtypedefs) $(FRC)

AddCmd.o:	$(INCcommandfile)
AddressMatch.o:	$(INCaddress)
Args.o:		$(INCArgs)
ArgsUsage.o:	$(INCArgs)
CanAddrMatch.o:	$(INCaddress)
ChangeCmd.o:	$(INCcommandfile)
CheckData.o:	$(INCcommandfile)
CheckDirs.o:	$(INCexec) $(INCPasswd)
CmdsData.o:	$(INCcommandfile)
CmdToString.o:	$(INCcommandfile)
CollectData.o:	$(INCcommandfile)
ConvSun3Cmds.o:	$(INCcommandfile)
CopyCmds.o:	$(INCcommandfile)
DaemonActive.o:	$(INCspool)
DecodeNum.o:	$(INCspool)
DoArgs.o:	$(INCArgs)
EncodeNum.o:	$(INCspool)
Execute.o:	$(INCexec)
Fowarded.o:	$(INCforward)
FreeCmds.o:	$(INCcommandfile)
GetErrFile.o:	$(INCsysexits)
GetFoward.o:	$(INCforward)
GetHandler.o:	$(INChandlers) $(INCspool)
GetInvoker.o:	$(INCPasswd)
GetNetUid.o:	$(INCPasswd)
GetParams.o:	$(INCparams)
GetPassword.o:	$(INCaddress) $(INCPasswd)
GetPrivs.o:	$(INCPasswd)
GetUser.o:	$(INCPasswd)
LinkCmds.o:	$(INCcommandfile) $(INCspool)
MailNCC.o:	$(INCexec) $(INCPasswd)
MakeErrFile.o:	$(INCspool)
MakeLicence.o:	$(INClicence)
MakeLock.o:	$(INCPasswd)
MkDirs.o:	$(INCexec)
MoveCmds.o:	$(INCcommandfile)
MsgsDB.o:	$(INCmsgsdb)
NodeName.o:	$(INClockfile)
RdStats.o:	$(INCstats)
ReadCmds.o:	$(INCcommandfile)
ReadFdCmds.o:	$(INCcommandfile)
ReRoute.o:	$(INCspool)
RmDir.o:	$(INCexec)
SetFoward.o:	$(INCforward) $(INCPasswd)
SetLock.o:	$(INClockfile) $(INCPasswd)
Shell.o:	$(INCexec) $(INCPasswd) $(INCspool)
StripCopyEnd.o:	$(INCaddress)
StripDomain.o:	$(INCaddress)
StripTypes.o:	$(INCaddress)
UniqueName.o:	$(INCspool)
Version.o:	$(INCstrings-data)
WrStats.o:	$(INCstats)
WriteCmds.o:	$(INCcommandfile)

version:
		@$(GET) -e Version.c >/dev/null
		@$(DELTA) -y"$(COMMENT)" Version.c >/dev/null
		@$(GET) Version.c >/dev/null

llib.ln:	llib-$(NAME) $(INCaddress) $(INCArgs) $(INCcommandfile) $(INCexec) \
		  $(INCforward) $(INCglobal) $(INChandlers) $(INCparams) $(INCPasswd) \
		  $(INCspool) $(INCstats)
		-/lib/cpp $(LFLAGS) -C llib-$(NAME) \
		| /usr/lib/lint/lint1 -v >llib.ln

lint:		lint.out
		$(LIST) lint.out

lint.out:	$(HEADERS) $(CFILES) $(LFILES) $(CSFILES)
		$(LINT) $(LFLAGS) $(CFILES) $(LFILES) $(CSFILES) \
		2>&1 | $(LSED) >$@

print:		Makefile llib-$(NAME) $(HEADERS) $(CFILES) $(LFILES) $(CSFILES)
		$(PRFORMATTER) Makefile llib-$(NAME) *.h *.c | $(PRSPOOL)

clean:
		$(RM) *.o crcs.s lint.out

clobber:	clean
		$(RM) llib.ln

FRC:
