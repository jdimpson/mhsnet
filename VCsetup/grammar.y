%{
/*
**	Copyright 2012 Piers Lauder
**
**	This file is part of MHSnet.
**
**	MHSnet is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	MHSnet is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with MHSnet.  If not, see <http://www.gnu.org/licenses/>.
*/


#define	STDIO

#include "global.h"
#include "call.h"

%}
%union {
	Symbol  *ysym;		/* ysymbol table pointer */
	char	*str;		/* string value */
	long	num;		/* integer */
	Op	*oper;		/* operation */
	Pat	*pattern;	/* pattern */
	Vlist	*vlist;		/* variable list */
}
%token	<ysym>	CALL CLOSE DAEMON DEVICE EXECD EXPECT FAIL FORKD FORKC
%token	<ysym>	MATCH MODE MONITOR OPEN READ READCHAR RETRY RETURN SHELL SLEEP
%token	<ysym>	TIME_OUT TRACE WRITE SLOWWRITE
%token	<ysym>	CRC ID NEXT SET TEST SHIFT
%token	<ysym>	NUMBER STRING
%type	<oper>	call close command crc daemon device expect match monitor
%type	<oper>	execd forkd forkc fail list mode next open read readchar
%type	<oper>	retry return set shell sleep statement
%type	<oper>	test timeout trace write slowwrite shift
%type	<pattern> alts alt
%type	<vlist> varlist
%type	<ysym>	var numvar
%%
list:   statement
		{ $$ = $1; }
	| list statement 
		{ $1->next = $2; $$ = $2;};
statement:
	command ';'
		{ $$ = $1; }
	| ID ':' command ';'
		{ $1->val.op = $3; $3->lab = $1; $$ = $3; };

command:
	  call
	| crc
	| close
	| daemon
	| device
	| execd
	| expect
	| fail
	| forkc
	| forkd
	| match
	| mode
	| monitor
	| next
	| open
	| read
	| readchar
	| retry
	| return
	| set
	| shell
	| shift
	| sleep
	| slowwrite
	| test
	| timeout
	| trace
	| write
		{ $$ = $1; };

call:		CALL		var	{ $$ = op($1->type); $$->op1.sym = $2; };
daemon:		DAEMON		var	{ $$ = op($1->type); $$->op1.sym = $2; };
execd:		EXECD		var	{ $$ = op($1->type); $$->op1.sym = $2; };
execd:		EXECD			{ $$ = op($1->type); $$->op1.sym = (Symbol *)0; };
forkc:		FORKC		var	{ $$ = op($1->type); $$->op1.sym = $2; };
forkd:		FORKD		var	{ $$ = op($1->type); $$->op1.sym = $2; };
forkd:		FORKD			{ $$ = op($1->type); $$->op1.sym = (Symbol *)0; };

read:		READ		ID	{ $$ = op($1->type); $$->op1.sym = $2; $2->vtype = STRING; };
readchar:	READCHAR	ID	{ $$ = op($1->type); $$->op1.sym = $2; $2->vtype = STRING; };

close:		CLOSE			{ $$ = op($1->type); };
return:		RETURN			{ $$ = op($1->type); };

monitor:	MONITOR		numvar	{ $$ = op($1->type); $$->op1.sym = $2; };
monitor:	MONITOR			{ $$ = op($1->type); $$->op1.sym = (Symbol *)0; };
retry:		RETRY		numvar	{ $$ = op($1->type); $$->op1.sym = $2; };
sleep:		SLEEP		numvar	{ $$ = op($1->type); $$->op1.sym = $2; };
timeout:	TIME_OUT	numvar	{ $$ = op($1->type); $$->op1.sym = $2; };

device:		DEVICE		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
fail:		FAIL		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
mode:		MODE		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
open:		OPEN		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
shell:		SHELL		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
slowwrite:	SLOWWRITE	varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
trace:		TRACE		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
write:		WRITE		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };
shift:		SHIFT		varlist	{ $$ = op($1->type); $$->op1.vlist = $2; };

varlist:	var			{ $$ = vlelt($1); }
	|	varlist	var		{ $$ = vlelt($2); $$->next = $1; $1->prev = $$; };

test:		TEST	ID	ID	{ $$ = op($1->type); $$->op1.sym = $2; $$->op2.sym = $3; };

crc:		CRC	ID	varlist {
			$$ = op($1->type);
			$$->op1.sym = $2;
			if ($2->vtype == NUMBER)
				warning($2->name, " is already a NUMBER\n");
			$2->vtype = STRING;
			$$->op2.vlist = $3;
		};
			
set:		SET	ID	varlist	{
			$$ = op($1->type);
			$$->op1.sym = $2;
			if ($2->vtype == NUMBER)
				warning($2->name, " is already a NUMBER\n");
			$2->vtype = STRING;
			$$->op2.vlist = $3;
		};

set:		SET	ID	NUMBER	{
			$$ = op($1->type);
			$$->op1.sym = $2;
			if ($2->vtype == STRING)
				warning($2->name, " is already a STRING\n");
			$2->vtype = NUMBER;
			$$->op2.sym = $3;
		};

next:		NEXT	ID		{ $$ = op($1->type); $$->op1.sym = $2; };
expect:		EXPECT	alts		{ $$ = op($1->type); $$->op1.pat = $2; };
match:		MATCH	var	alts	{ $$ = op($1->type); $$->op1.sym = $2; $$->op2.pat = $3; };

alts:		alt			{ $$ = $1; }
	|	alt	','	alts	{ $1->next = $3; $$ = $1; };

alt:		var	ID		{ $$ = pat($1, $2); };

var:		ID
	|	STRING			{ $$ = $1; };

numvar:		ID
	|	NUMBER			{ $$ = $1; };
%%
