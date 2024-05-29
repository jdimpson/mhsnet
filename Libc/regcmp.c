	/*LINTLIBRARY*/

#define	STDARGS

#include	"global.h"
#include	"regdefs.h"

#define SLOP	5

int	__i_size;

/*VARARGS*/
char *
#ifdef	ANSI_C
regcmp(char *sp, ...)
{
#else	/* ANSI_C */
regcmp(va_alist) 
	va_dcl
{
	register char	*sp;
#endif	/* ANSI_C */
	register int	c;
	register char	*ep;
	register char	**adx;
	va_list		vp;
	char		*lastep;
	char		*sep;
	int		nbra;
	int		ngrp;
	char		*stack[SSIZE];
	char		*args[NBRA+1];
	char		**esp;

	esp = stack;
	adx = args;
	c = nbra = ngrp = 0;

#	ifdef	ANSI_C
	va_start(vp, sp);
	while (sp != NULLSTR) {
		*adx++ = sp;
		do
			++c;
		while (*sp++);
		sp = va_arg(vp, char *);
	}
#	else	/* ANSI_C */
	va_start(vp);
	while (sp = va_arg(vp, char *)) {
		*adx++ = sp;
		do
			++c;
		while (*sp++);
	}
#	endif	/* ANSI_C */
	*adx = (char *)0;
	va_end(vp);

	adx = args;
	sp = *adx++;
	if ((ep = malloc((unsigned)(2*c + SLOP))) == (char *)0)
		return ep;
	sep = ep;
	if ((c = *sp++) == NULCH) goto cerror;
	if (c == '^') {
		c = *sp++;
		*ep++ = CIRCFL;
	}
	if (c == '*' || c == '+' || c == '{')
		goto cerror;
	sp--;
	for (;;) {
		if ((c = *sp++) == NULCH) {
			if (*adx) {
				sp = *adx++;
				continue;
			}
			*ep++ = CEOF;
			if (--nbra > NBRA || esp != stack)
				goto cerror;
			__i_size = ep - sep;
			return sep;
		}
		if (c != '*' && c != '{' && c != '+')
			lastep = ep;
		switch (c)
		{
		case '(':
			if (esp == &stack[SSIZE]) goto cerror;
			*esp++ = ep;
			*ep++ = CBRA;
			*ep++ = 0xff;
			continue;

		case ')':
		    {
			register char	*eptr;
			register int	cclcnt;

			if (esp == stack) goto cerror;
			eptr = *--esp;
			if ((c = *sp++) == '$') {
				if ('0' > (c = *sp++) || c > '9')
					goto cerror;
				*ep++ = CKET;
				*ep++ = *++eptr = nbra++;
				*ep++ = (c-'0');
				continue;
			}
			*ep++ = EGRP;
			*ep++ = ngrp++;
			sp--;
			switch (c)
			{
			case '+':
				*eptr = PGRP;
				break;
			case '*':
				*eptr = SGRP;
				break;
			case '{':
				*eptr = TGRP;
				break;
			default:
				*eptr = GRP;
				continue;
			}
			c = ep - eptr - 2;
			for (cclcnt = 0; c >= 256; cclcnt++)
				c -= 256;
			if (cclcnt > 3) goto cerror;
			*eptr |= cclcnt;
			*++eptr = c;
			continue;
		    }

		case '\\':
			*ep++ = CCHR;
			if ((c = *sp++) == NULCH)
				goto cerror;
			*ep++ = c;
			continue;

		case '{':
		    {
			register int i;
			register int cflg;

			*lastep |= RNGE;
			cflg = 0;
		nlim:
			if ((c = *sp++) == '}') goto cerror;
			i = 0;
			do {
				if ('0' <= c && c <= '9')
					i = (i*10+(c-'0'));
				else goto cerror;
			} while (((c = *sp++) != '}') && (c != ','));
			if (i > 255) goto cerror;
			*ep++ = i;
			if (c == ',') {
				if (cflg++) goto cerror;
				if ((c = *sp++) == '}') {
					*ep++ = 0xff;
					continue;
				}
				else {
					sp--;
					goto nlim;
				}
			}
			if (!cflg) *ep++ = i;
			else if ((ep[-1]&0377) < (ep[-2]&0377)) goto cerror;
			continue;
		    }
		case '.':
			*ep++ = CDOT;
			continue;

		case '+':
			if (*lastep == CBRA || *lastep == CKET)
				goto cerror;
			*lastep |= PLUS;
			continue;

		case '*':
			if (*lastep == CBRA || *lastep == CKET)
			goto cerror;
			*lastep |= STAR;
			continue;

		case '$':
			if (*sp != NULCH || *adx)
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
		    {
			register int	cclcnt;

			if ((c = *sp++) == '^') {
				c = *sp++;
				*ep++ = NCCL;
			}
			else
				*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			do {
				if (c == NULCH)
					goto cerror;
				if (c == '-' && cclcnt > 1 && *sp != ']') {
					c = ep[-1];
					ep[-1] = MINUS;
				}
				*ep++ = c;
				cclcnt++;
			} while ((c = *sp++) != ']');
			lastep[1] = cclcnt;
			continue;
		    }

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
   cerror:
	free(sep);
	return (char *)0;
}
