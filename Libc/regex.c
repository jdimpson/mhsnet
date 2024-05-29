	/*LINTLIBRARY*/
/*
**	SCCSID	@(#)regex.c	1.8 05/12/16
*/

#define	STDARGS

#include	"global.h"
#include	"regdefs.h"

char	*__braslist[NBRA];
char	*__braelist[NBRA];
char	*__loc1;
int	__bravar[NBRA];
char	*__st[SSIZE + 1];
char	**__eptr_,**__lptr_;
int	__cflg;

static char	*__advance _FA_((char *, char *));
static int	__cclass _FA_((char *, char, int));
static char	*__execute _FA_((char *, char *));
static void	__getrnge _FA_((int *, int *, char *));
static char	*__xpop _FA_((int));
static void	__xpush _FA_((int, char *));

/*VARARGS*/
char *
#ifdef	ANSI_C
regex(char *addrc, char *addrl, ...)
{
#else	/* ANSI_C */
regex(va_alist)
	va_dcl
{
	char *addrc, *addrl;
#endif	/* ANSI_C */
	register char *p1, *p2;
	register int in;
	char *cur;
	va_list vp;
	char *args[NBRA];

#	ifdef	ANSI_C
	va_start(vp, addrl);
#	else	/* ANSI_C */
	va_start(vp);
	addrc = va_arg(vp, char *);
	addrl = va_arg(vp, char *);
#	endif	/* ANSI_C */
	for(in=0;in<NBRA;in++) {
		__braslist[in] = 0;
		__bravar[in] = -1;
		args[in] = va_arg(vp, char *);
	}
	va_end(vp);
	__cflg = 0;
	cur = __execute(addrc,addrl);
	for(in=0;in<NBRA;in++) {
		if ((p1 = __braslist[in]) && (__bravar[in] >= 0)) {
			p2 = args[__bravar[in]];
			while(p1 < __braelist[in]) *p2++ = *p1++;
			*p2 = '\0';
		}
	}
	if (!__cflg) return (addrl==cur)?(char *)0:cur;
	else return cur;
}

static char *
__execute(addrc,addrl)
char *addrc,*addrl;
{
	register char *p1, *p2, c;
	char *i;

	p1 = addrl;
	p2 = addrc;
	__eptr_ = &__st[SSIZE];
	__lptr_ = &__st[0];
	if (*p2==CIRCFL) {
		__loc1 = p1;
		return (i=__advance(p1,++p2))?i:addrl;
	}
	/* fast check for first character */
	if (*p2==CCHR) {
		c = p2[1];
		do {
			if (*p1!=c)
				continue;
			__eptr_ = &__st[SSIZE];
			__lptr_ = &__st[0];
			if ( (i = __advance(p1, p2)) )  {
				__loc1 = p1;
				return(i);
			}
		} while (*p1++);
		return(addrl);
	}
	/* regular algorithm */
	do {
		__eptr_ = &__st[SSIZE];
		__lptr_ = &__st[0];
		if ( (i = __advance(p1, p2)) )  {
			__loc1 = p1;
			return(i);
		}
	} while (*p1++);
	return(addrl);
}

static char *
__advance(lp, ep)
	register char *lp, *ep;
{
	register char *curlp;
	char *sep,*dp;
	int i,lcnt,dcnt,gflg;

	gflg = 0;
	for (;;) switch(*ep++)
	{
	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return((char *)0);

	case EGRP|RNGE:
		return(lp);
	case EGRP:
	case GRP:
		ep++;
		continue;

	case EGRP|STAR:
		(void)__xpop(0);
	case EGRP|PLUS:
		__xpush(0,++ep);
		return(lp);

	case CDOT:
		if (*lp++)
			continue;
		return((char *)0);

	case CDOL:
		if (*lp==0)
			continue;
		lp++;
		return((char *)0);

	case CEOF:
		__cflg = 1;
		return(lp);

	case TGRP:
	case TGRP|A768:
	case TGRP|A512:
	case TGRP|A256:
		i = (((ep[-1]&03)<<8) + ((*ep)&0377));
		ep++;
		__xpush(0,ep + i + 2);
		gflg = 1;
		__getrnge(&lcnt,&dcnt,&ep[i]);
		while(lcnt--)
			if (!(lp=__advance(lp,ep)))
				return((char *)0);
		__xpush(1,curlp=lp);
		while(dcnt--)
			if(!(dp=__advance(lp,ep))) break;
			else __xpush(1,lp=dp);
		ep = __xpop(0);
		goto star;
	case CCHR|RNGE:
		sep = ep++;
		__getrnge(&lcnt,&dcnt,ep);
		while(lcnt--)
			if(*lp++!=*sep) return((char *)0);
		curlp = lp;
		while(dcnt--)
			if(*lp++!=*sep) break;
		if (dcnt < 0) lp++;
		ep += 2;
		goto star;
	case CDOT|RNGE:
		__getrnge(&lcnt,&dcnt,ep);
		while(lcnt--)
			if(*lp++ == '\0') return((char *)0);
		curlp = lp;
		while(dcnt--)
			if(*lp++ == '\0') break;
		if (dcnt < 0) lp++;
		ep += 2;
		goto star;
	case CCL|RNGE:
	case NCCL|RNGE:
		__getrnge(&lcnt,&dcnt,(ep + (*ep&0377)));
		while(lcnt--)
			if(!__cclass(ep,*lp++,ep[-1]==(CCL|RNGE))) return((char *)0);
		curlp = lp;
		while(dcnt--)
			if(!__cclass(ep,*lp++,ep[-1]==(CCL|RNGE))) break;
		if (dcnt < 0) lp++;
		ep += (*ep + 2);
		goto star;
	case CCL:
		if (__cclass(ep, *lp++, 1)) {
			ep += *ep;
			continue;
		}
		return((char *)0);

	case NCCL:
		if (__cclass(ep, *lp++, 0)) {
			ep += *ep;
			continue;
		}
		return((char *)0);

	case CBRA:
		__braslist[(int)*ep++] = lp;
		continue;

	case CKET:
		__braelist[(int)*ep] = lp;
		__bravar[(int)*ep] = ep[1];
		ep += 2;
		continue;

	case CDOT|PLUS:
		if (*lp++ == '\0') return((char *)0);
	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;

	case CCHR|PLUS:
		if (*lp++ != *ep) return((char *)0);
	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case PGRP:
	case PGRP|A256:
	case PGRP|A512:
	case PGRP|A768:
		if (!(lp=__advance(lp,ep+1))) return((char *)0);
	case SGRP|A768:
	case SGRP|A512:
	case SGRP|A256:
	case SGRP:
		i = (((ep[-1]&03) << 8) + (*ep&0377));
		ep++;
		__xpush(0,ep + i);
		__xpush(1,curlp=lp);
		{
			char *tempp;
			while ( (tempp = __advance(lp,ep)) )
				__xpush(1,lp=tempp);
		}
		ep = __xpop(0);
		gflg = 1;
		goto star;

	case CCL|PLUS:
	case NCCL|PLUS:
		if (!__cclass(ep,*lp++,ep[-1]==(CCL|PLUS))) return((char *)0);
	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (__cclass(ep, *lp++, ((ep[-1]==(CCL|STAR)) || (ep[-1]==(CCL|PLUS)))));
		ep += *ep;
		goto star;

	star:
		do {
			char *tempp;
			if(!gflg) lp--;
			else if (!(lp=__xpop(1))) break;
			if ( (tempp = __advance(lp, ep)) ) 
				return(tempp);
		} while (lp > curlp);
		return((char *)0);

	default:
		return((char *)0);
	}
}

static int
#if	ANSI_C
__cclass(char *set, char c, int af)
#else	/* ANSI_C */
__cclass(set, c, af)
register char *set, c;
int	af;
#endif	/* ANSI_C */
{
	register int n;

	if (c == 0)
		return 0;
	n = *set++;
	while (--n) {
		if (*set == MINUS) {
			if (set[2] < set[1]) return 0;
			if (*++set <= c) {
				if (c <= *++set)
					return af;
			}
			else ++set;
			++set;
			n -= 2;
			continue;
		}
		if (*set++ == c)
			return af;
	}
	return !af;
}

static void
__xpush(i,p)
int i;
char *p;
{
	if (__lptr_ >= __eptr_)
	{
		extern void exit();

		(void)write(2, "regex stack overflow\n",15);
		exit(1);
	}
	if (i) *__lptr_++ = p;
	else   *__eptr_-- = p;
}

static char *
__xpop(i)
int i;
{
	if (i)
		return (__lptr_ < &__st[0])?(char *)0:*--__lptr_;
	else
		return (__eptr_ > &__st[SSIZE])?(char *)0:*++__eptr_;
}

static void
__getrnge(i,j,k)
int *i,*j;
char *k;
{
	*i = *k++ & 0xff;
	*j = *k & 0xff;
	if (*j == 0xff)
		*j = 20000;
	else
		*j -= *i;
}
