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


#ifndef	AS
#include	"global.h"
#else	/* AS */
#include	"site.h"
#endif	/* AS */

/*
**	Packet check bytes calculation
**
**	CRC16:  x**16 + x**15 + x**2 + 1
*/

#define	C_VERSION	/* C-language version will be compiled unless undefined below */
#define	C_TABLE		/* C-language lookup table will be compiled unless undefined below */

/*
**	There is a test program in Admin/test-crc.c to aid you
**	in developing your own assembler version.
**
**	If your "vax" is a microvax, it does not have a crc instuction.
**	CONFIG in your Makefile or run file should include -Duvax.
*/

#ifdef	uvax
#undef	vax
#endif

#ifdef	vax

#undef	C_VERSION
#undef	C_TABLE

#ifdef	AS

	.text
	.align	1
	.globl	null
null:	.word	0
	ret

#else	/* AS */

/*
**	Vax "crc" instruction look-up table for polynomial = 0120001
*/

long	crc16t[] =
{
	       0, 0146001, 0154001,  012000, 0170001,  036000,  024000, 0162001
	,0120001,  066000,  074000, 0132001,  050000, 0116001, 0104001,  042000
};

/*ARGSUSED*/
bool
crc(s, n)
	char *	s;
	int	n;
{
	asm("	crc	_crc16t,$0,8(ap),*4(ap)	");
	asm("	cmpw	r0,(r3)			");
	asm("	beqlu	OK			");
	asm("	movw	r0,(r3)			");
	asm("	movl	$1,r0			");
	asm("	ret				");
	asm("OK:movw	r0,(r3)			");
	return false;
}

/*ARGSUSED*/
Crc_t
acrc(c, s, n)
	Crc_t	c;
	char *	s;
	int	n;
{
	asm("	crc	_crc16t,4(ap),12(ap),*8(ap)	");
	asm("	ret					");
#	ifdef	lint
	return c;
#	endif
}

/*ARGSUSED*/
bool
tcrc(s, n)
	char *	s;
	int	n;
{
	asm("	crc	_crc16t,$0,8(ap),*4(ap)	");
	asm("	cmpw	r0,(r3)			");
	asm("	beqlu	TK			");
	asm("	movl	$1,r0			");
	asm("	ret				");
	asm("TK:				");
	return false;
}

#endif	/* AS */

#else	/* vax */

#ifdef	pdp11

#undef	C_VERSION

#ifdef	AS

/*
**	bool crc(string, length)
**	char *string;
*/
	.globl	_crc16t_
	.globl	_crc
_crc:
	len=r0
	crc=r1
	str=r2
	byt=r3

	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	6(sp),str
	mov	8.(sp),len
	clr	crc
1:
	movb	(str)+,byt
	xor	crc,byt
	bic	$177400,byt
	asl	byt
	mov	_crc16t_(byt),byt
	clrb	crc
	swab	crc
	xor	byt,crc
	sob	len,1b

	cmpb	crc,*str
	jeq	1f
	inc	len
1:
	movb	crc,(str)+
	swab	crc
	cmpb	crc,*str
	jeq	1f
	inc	len
1:
	movb	crc,*str
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc
/*
**	Crc_t acrc(crc, string, length)
**	Crc_t crc;
**	char *string;
*/
	.globl	_acrc
_acrc:
	crc=r0
	len=r1
	str=r2
	byt=r3

	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	6(sp),crc
	mov	8.(sp),str
	mov	10.(sp),len
1:
	movb	(str)+,byt
	xor	crc,byt
	bic	$177400,byt
	asl	byt
	mov	_crc16t_(byt),byt
	clrb	crc
	swab	crc
	xor	byt,crc
	sob	len,1b

	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc
/*
**	bool tcrc(string, length)
**	char *string;
*/
	.globl	_crc16t_
	.globl	_tcrc
_tcrc:
	len=r0
	crc=r1
	str=r2
	byt=r3

	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	6(sp),str
	mov	8.(sp),len
	clr	crc
1:
	movb	(str)+,byt
	xor	crc,byt
	bic	$177400,byt
	asl	byt
	mov	_crc16t_(byt),byt
	clrb	crc
	swab	crc
	xor	byt,crc
	sob	len,1b

	cmpb	crc,(str)+
	jeq	1f
	inc	len
1:
	swab	crc
	cmpb	crc,*str
	jeq	1f
	inc	len
1:
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc

#endif	/* AS */

#else	/* pdp11 */

#ifdef	mc68000

#ifdef	BELL_68K

#undef	C_VERSION

#ifdef	AS

/*
**	If your 680X0 C compiler uses 16 bit ints,
**	then you should define INT16BIT.
**
**	Bell MC68000 assembler for PCC2.
*/

/*
**	bool crc(string, length)
**	char *string;
*/
	text
	set	crcF,0
#	define	len	%d0
#	define	crca	%d1
#	define	wrd	%d2
#	define	str	%a0
#	define	tbl	%a1

	global	crc16t_
	global	crc
crc:
	link	%fp,&crcF
	mov.l	%d2,-(%sp)
	mov.l	8(%fp),str
	clr.w	crca
#ifdef	INT16BIT
	mov.w	12(%fp),len
#else	/* INT16BIT */
	mov.w	14(%fp),len
#endif	/* INT16BIT */
	ble	crc%140
	sub.w	&1,len
	mov.l	&crc16t_,tbl
crc%170:
	clr.w	wrd
	mov.b	(str)+,wrd
	eor.b	crca,wrd
	add.w	wrd,wrd
	mov.w	0(tbl,wrd.w),wrd
	lsr.w	&8,crca
	eor.w	wrd,crca
	dbra	len,crc%170
crc%140:
	clr.l	len
	cmp.b	crca,(str)
	beq	crc%180
	mov.l	&1,len
crc%180:
	mov.b	crca,(str)+
	lsr.w	&8,crca
	cmp.b	crca,(str)
	beq	crc%190
	mov.l	&1,len
crc%190:
	mov.b	crca,(str)
	mov.l	(%sp)+,%d2
	unlk	%fp
	rts

#	undef	crca
#	undef	len
#	undef	wrd
#	undef	str
#	undef	tbl

/*
**	Crc_t acrc(crc, string, length)
**	Crc_t crc;
**	char *string;
*/
	text
	set	crcF,0
#	define	crca	%d0
#	define	len	%d1
#	define	wrd	%d2
#	define	str	%a0
#	define	tbl	%a1

	global	crc16t_
	global	acrc
acrc:
	link	%fp,&crcF
	mov.l	%d2,-(%sp)
	mov.w	10(%fp),crca
#ifdef	INT16BIT
	mov.w	16(%fp),len
#else	/* INT16BIT */
	mov.w	18(%fp),len
#endif	/* INT16BIT */
	ble	crc%240
	sub.w	&1,len
	mov.l	12(%fp),str
	mov.l	&crc16t_,tbl
crc%270:
	clr.w	wrd
	mov.b	(str)+,wrd
	eor.b	crca,wrd
	add.w	wrd,wrd
	mov.w	0(tbl,wrd.w),wrd
	lsr.w	&8,crca
	eor.w	wrd,crca
	dbra	len,crc%270
crc%240:
	mov.l	(%sp)+,%d2
	unlk	%fp
	rts

#	undef	crca
#	undef	len
#	undef	wrd
#	undef	str
#	undef	tbl

/*
**	bool tcrc(string, length)
**	char *string;
*/
	text
	set	crcF,0
#	define	len	%d0
#	define	crca	%d1
#	define	wrd	%d2
#	define	str	%a0
#	define	tbl	%a1

	global	crc16t_
	global	tcrc
tcrc:
	link	%fp,&crcF
	mov.l	%d2,-(%sp)
	mov.l	8(%fp),str
	clr.w	crca
#ifdef	INT16BIT
	mov.w	12(%fp),len
#else	/* INT16BIT */
	mov.w	14(%fp),len
#endif	/* INT16BIT */
	ble	crc%340
	sub.w	&1,len
	mov.l	&crc16t_,tbl
crc%370:
	clr.w	wrd
	mov.b	(str)+,wrd
	eor.b	crca,wrd
	add.w	wrd,wrd
	mov.w	0(tbl,wrd.w),wrd
	lsr.w	&8,crca
	eor.w	wrd,crca
	dbra	len,crc%370
crc%340:
	clr.l	len
	cmp.b	crca,(str)+
	beq	crc%380
	mov.l	&1,len
crc%380:
	lsr.w	&8,crca
	cmp.b	crca,(str)
	beq	crc%390
	mov.l	&1,len
crc%390:
	mov.l	(%sp)+,%d2
	unlk	%fp
	rts

#endif	/* AS */

#endif	/* BELL_68K */

#ifdef	MIT_68K

#undef	C_VERSION

#ifdef	AS

/*
**	MIT assembler version.
*/
	.text
	.globl	_crc16t_
/*
**	bool crc(string, length)
**	char *string;
*/
	.globl	_crc

	LF13 = 0
#	define	len	d0
#	define	crca	d1
#	define	wrd	d2
#	define	byt	d3
#	define	str	a0
#	define	tbl	a1

_crc:
	link	a6,#LF13
	movl	d2,sp@-
#	ifdef	mc68020
	movl	d3,sp@-
#	endif	/* mc68020 */
	movl	a6@(8),str
	clrw	crca
	movw	a6@(14),len
	jle	L15
	subqw	#1,len
	movl	#_crc16t_,tbl
#	ifdef	mc68020
	clrw	wrd
	clrw	byt
#	endif	/* mc68020 */
L18:
#	ifndef	mc68020
	clrw	wrd
	movb	str@+,wrd
	eorb	crca,wrd
	addw	wrd,wrd
	movw	tbl@(0,wrd:W),wrd
#	else	/* mc68020 */
	movb	str@+,byt
	eorb	crca,byt
	movw	tbl@(0,byt:W:2),wrd
#	endif	/* mc68020 */
	lsrw	#8,crca
	eorw	wrd,crca
	dbra	len,L18
L15:
	clrl	len
	cmpb	str@,crca
	jeq	L19
	movl	#1,len
L19:
	movb	crca,str@+
	lsrw	#8,crca
	cmpb	str@,crca
	jeq	L20
	movl	#1,len
L20:
	movb	crca,str@
#	ifdef	mc68020
	movl	sp@+,d3
#	endif	/* mc68020 */
	movl	sp@+,d2
	unlk	a6
	rts

#	undef	crca
#	undef	len
#	undef	wrd
#	undef	byt
#	undef	str
#	undef	tbl

/*
**	Crc_t acrc(crc, string, length)
**	Crc_t crc;
**	char *string;
*/
	.globl	_acrc

	LF22 = 0
#	define	crca	d0
#	define	len	d1
#	define	wrd	d2
#	define	byt	d3
#	define	str	a0
#	define	tbl	a1

_acrc:
	link	a6,#LF22
	movl	d2,sp@-
#	ifdef	mc68020
	movl	d3,sp@-
#	endif	/* mc68020 */
	movw	a6@(10),crca
	movw	a6@(18),len
	jle	L24
	subqw	#1,len
	movl	a6@(12),str
	movl	#_crc16t_,tbl
#	ifdef	mc68020
	clrw	wrd
	clrw	byt
#	endif	/* mc68020 */
L27:
#	ifndef	mc68020
	clrw	wrd
	movb	str@+,wrd
	eorb	crca,wrd
	addw	wrd,wrd
	movw	tbl@(0,wrd:W),wrd
#	else	/* mc68020 */
	movb	str@+,byt
	eorb	crca,byt
	movw	tbl@(0,byt:W:2),wrd
#	endif	/* mc68020 */
	lsrw	#8,crca
	eorw	wrd,crca
	dbra	len,L27
L24:
#	ifdef	mc68020
	movl	sp@+,d3
#	endif	/* mc68020 */
	movl	sp@+,d2
	unlk	a6
	rts

#	undef	crca
#	undef	len
#	undef	wrd
#	undef	byt
#	undef	str
#	undef	tbl

/*
**	bool tcrc(string, length)
**	char *string;
*/
	.globl	_tcrc

	LF13 = 0
#	define	len	d0
#	define	crca	d1
#	define	wrd	d2
#	define	byt	d3
#	define	str	a0
#	define	tbl	a1

_tcrc:
	link	a6,#LF13
	movl	d2,sp@-
#	ifdef	mc68020
	movl	d3,sp@-
#	endif	/* mc68020 */
	movl	a6@(8),str
	clrw	crca
	movw	a6@(14),len
	jle	L25
	subqw	#1,len
	movl	#_crc16t_,tbl
#	ifdef	mc68020
	clrw	wrd
	clrw	byt
#	endif	/* mc68020 */
L28:
#	ifndef	mc68020
	clrw	wrd
	movb	str@+,wrd
	eorb	crca,wrd
	addw	wrd,wrd
	movw	tbl@(0,wrd:W),wrd
#	else	/* mc68020 */
	movb	str@+,byt
	eorb	crca,byt
	movw	tbl@(0,byt:W:2),wrd
#	endif	/* mc68020 */
	lsrw	#8,crca
	eorw	wrd,crca
	dbra	len,L28
L25:
	clrl	len
	cmpb	str@+,crca
	jeq	L29
	movl	#1,len
L29:
	lsrw	#8,crca
	cmpb	str@,crca
	jeq	L30
	movl	#1,len
L30:
#	ifdef	mc68020
	movl	sp@+,d3
#	endif	/* mc68020 */
	movl	sp@+,d2
	unlk	a6
	rts

#endif	/* AS */

#endif	/* MIT_68K */

#else	/* mc68000 */

#ifdef	ns32000

#ifdef	sequent

#undef	C_VERSION

#ifdef	AS

/*
**	bool crc(string,length)
**	char	*string;
*/

#define	str	r7
#define	byt	r6
#define	crc	r1
#define	len	r0

	.text
	.globl	_crc16t_
	.globl	_crc
	.align	2
_crc:
	enter	[r7,r6],0
	movd	8(fp), str
	movd	12(fp), len
	cmpqd	0,	len
	beq	L2
	movqd	0,	crc
L1:
	movb	0(str),	byt
	addqd	1, str
	xorw	crc, byt
	bicd	0xffffff00, byt
	movzwd	_crc16t_[byt:w], byt
	bicb	0x0ff, crc
	rotw	8, crc
	xorw	byt, crc
	acbd	-1, len, L1
L2:
	cmpw	crc, 0(str)
	beq	L3
	movqd	1, len
L3:
	movw	crc, 0(str)
	exit	[r7,r6]
	ret	0

#undef	str
#undef	byt
#undef	crc
#undef	len

/*
**	Crc_t acrc(crc, string, length)
**	Crc_t crc;
**	char *string;
*/

#define	astr	r7
#define	abyt	r6
#define	alen	r1
#define	acrc	r0

	.align	2
	.globl	_acrc
_acrc:
	enter	[r7,r6],0
	movzwd	8(fp), acrc
	movd	12(fp), astr
	movd	16(fp), alen
L5:
	movb	0(astr), abyt
	addqd	1, astr
	xorw	acrc, abyt
	bicd	0xffffff00,abyt
	movzwd	_crc16t_[abyt:w], abyt
	bicb	0xff, acrc
	rotw	8, acrc
	xorw	abyt, acrc
	acbd	-1, alen, L5
	exit	[r7,r6]
	ret	0

#undef	astr
#undef	abyt
#undef	alen
#undef	acrc

/*
**	bool tcrc(string,length)
**	char	*string;
*/

#define	str	r7
#define	byt	r6
#define	crc	r1
#define	len	r0

	.text
	.globl	_crc16t_
	.globl	_tcrc
	.align	2
_tcrc:
	enter	[r7,r6],0
	movd	8(fp), str
	movd	12(fp), len
	cmpqd	0,	len
	beq	L12
	movqd	0,	crc
L11:
	movb	0(str),	byt
	addqd	1, str
	xorw	crc, byt
	bicd	0xffffff00, byt
	movzwd	_crc16t_[byt:w], byt
	bicb	0x0ff, crc
	rotw	8, crc
	xorw	byt, crc
	acbd	-1, len, L11
L12:
	cmpw	crc, 0(str)
	beq	L13
	movqd	1, len
L13:
	exit	[r7,r6]
	ret	0

#endif	/* AS */

#endif	/* sequent */

#else	/* ns32000 */

#ifdef	mips

#undef	C_VERSION

#ifdef	AS

/*
**	bool crc(string,length)
**	char *string;
*/

#	define	str	$4
#	define	tbl	$2	/* MUST be $2 - used for the return value */
#	define	crca	$3
#	define	len	$5
#	define	wrd	$8
#	define	tmp	$9
#	define	tmp1	$10

	.set noreorder
	.extern crc16t_	0
	.text
	.align	2
	.file	2 "crcs.s"
	.loc	2 1
	.globl	crc
	.ent	crc 2
crc:
	.option O4
	blez	len,$33
	move	crca,$0
	la	tbl,crc16t_
$32:
	lbu	wrd,0(str)
	srl	tmp,crca,8
	xor	wrd,wrd,crca
	sll	wrd,wrd,1
	andi	wrd,wrd,0x1fe
	add	wrd,wrd,tbl
	lhu	wrd,0(wrd)
	addiu	str,str,1
	addiu	len,len,-1
	bgtz	len,$32
	xor	crca,wrd,tmp
$33:
	lbu	tmp1,0(str)
	andi	tmp,crca,0xff
	andi	tmp1,tmp1,0xff
	beq	tmp,tmp1,$34
	move	tbl,$0
	ori	tbl,$0,1
$34:
	sb	crca,0(str)
	lbu	tmp,1(str)
	srl	crca,crca,8
	beq	tmp,crca,$35
	sb	crca,1(str)
	ori	tbl,$0,1
$35:
	j	$31
	nop
	.end

#	undef	str
#	undef	tbl
#	undef	crca
#	undef	len
#	undef	wrd
#	undef	tmp
#	undef	tmp1

/*
**	Crc_t acrc(crc, string, length)
**	Crc_t crc;
**	char *string;
*/

#	define	str	$5
#	define	crca	$4
#	define	tbl	$3
#	define	len	$6
#	define	wrd	$8
#	define	tmp	$9

	.text
	.align	2
	.file	2 "crcs.s"
	.loc	2 2
	.globl	acrc
	.ent	acrc 2
acrc:
	.option O4
	blez	len,$43
	la	tbl,crc16t_
$42:
	lbu	wrd,0(str)
	srl	tmp,crca,8
	xor	wrd,wrd,crca
	sll	wrd,wrd,1
	andi	wrd,wrd,0x1fe
	add	wrd,wrd,tbl
	lhu	wrd,0(wrd)
	addiu	str,str,1
	addiu	len,len,-1
	bgtz	len,$42
	xor	crca,wrd,tmp
$43:
	j	$31
	move	$2,crca
	.end

#	undef	str
#	undef	tbl
#	undef	crca
#	undef	len
#	undef	wrd
#	undef	tmp

/*
**	bool tcrc(string,length)
**	char	*string;
*/

#	define	str	$4
#	define	tbl	$2	/* MUST be $2 - used for the return value */
#	define	crca	$3
#	define	len	$5
#	define	wrd	$8
#	define	tmp	$9
#	define	tmp1	$10

	.text
	.align	2
	.file	2 "crcs.s"
	.loc	2 3
	.globl	tcrc
	.ent	tcrc 2
tcrc:
	.option O4
	blez	len,$53
	move	crca,$0
	la	tbl,crc16t_
$52:
	lbu	wrd,0(str)
	srl	tmp,crca,8
	xor	wrd,wrd,crca
	sll	wrd,wrd,1
	andi	wrd,wrd,0x1fe
	add	wrd,wrd,tbl
	lhu	wrd,0(wrd)
	addiu	str,str,1
	addiu	len,len,-1
	bgtz	len,$52
	xor	crca,wrd,tmp
$53:
	lbu	tmp1,0(str)
	andi	tmp,crca,0xff
	andi	tmp1,tmp1,0xff
	beq	tmp,tmp1,$54
	move	tbl,$0
	ori	tbl,$0,1
$54:
	lbu	tmp,1(str)
	srl	crca,crca,8
	beq	tmp,crca,$55
	nop
	ori	tbl,$0,1
$55:
	j	$31
	nop
	.end

	.set reorder

#endif	/* AS */

#else	/* mips */

#if	defined(i386) && SYSVREL >= 4 && linux == 0

#undef	C_VERSION

#ifdef	AS
	.file	"crc.c"
	.version	"01.01"
	.text
	.type	crc,@function
	.text
	.globl	crc
	.align	4

	.nopsets	"cc"
	.align	16
crc:
	pushl	%esi
	pushl	%ebx
	pushl	%ebp
	movl	20(%esp),%esi
	movl	16(%esp),%ebx
	xorl	%edx,%edx
	xorl	%eax,%eax
	leal	crc16t_,%ebp
	shrl	$1,%esi
	jnc	.L214
	xorl	%ecx,%ecx
	jmp	.L216
	.align	16,7,4
.L215:
	xorl	%ecx,%ecx
	movb	%dh,%cl
	xorb	(%ebx),%dl
	movb	%dl,%al
	xorw	(%ebp,%eax,2),%cx
	incl	%ebx
.L216:
	xorl	%edx,%edx
	movb	%ch,%dl
	xorb	(%ebx),%cl
	movb	%cl,%al
	xorw	(%ebp,%eax,2),%dx
	incl	%ebx
.L214:
	decl	%esi
	jge	.L215
	xorl	%eax,%eax
	cmpb	(%ebx),%dl
	je	.L218
	incl	%eax
.L218:
	movb	%dl,(%ebx)
	incl	%ebx
	cmpb	(%ebx),%dh
	je	.L219
	movb	$1,%al
.L219:
	movb	%dh,(%ebx)
	popl	%ebp
	popl	%ebx
	popl	%esi
	ret	
	.align	16,7,4
	.size	crc,.-crc
	.type	acrc,@function
	.text
	.globl	acrc
	.align	4

	.align	16
acrc:
	pushl	%esi
	pushl	%ebx
	pushl	%ebp
	movl	16(%esp),%eax
	movl	24(%esp),%esi
	movl	20(%esp),%ebx
	xorl	%edx,%edx
	leal	crc16t_,%ebp
	shrl	$1,%esi
	jnc	.L223
	movl	%eax,%ecx
	jmp	.L225
	.align	16,7,4
.L224:
	xorl	%ecx,%ecx
	movb	%ah,%cl
	xorb	(%ebx),%al
	movb	%al,%dl
	xorw	(%ebp,%edx,2),%cx
	incl	%ebx
.L225:
	xorl	%eax,%eax
	movb	%ch,%al
	xorb	(%ebx),%cl
	movb	%cl,%dl
	xorw	(%ebp,%edx,2),%ax
	incl	%ebx
.L223:
	decl	%esi
	jge	.L224
	popl	%ebp
	popl	%ebx
	popl	%esi
	ret	
	.align	16,7,4
	.size	acrc,.-acrc
	.type	tcrc,@function
	.text
	.globl	tcrc
	.align	4

	.align	16
tcrc:
	pushl	%esi
	pushl	%ebx
	pushl	%ebp
	movl	20(%esp),%esi
	movl	16(%esp),%ebx
	xorl	%edx,%edx
	xorl	%eax,%eax
	leal	crc16t_,%ebp
	shrl	$1,%esi
	jnc	.L230
	xorl	%ecx,%ecx
	jle	.L232
	.align	16,7,4
.L231:
	xorl	%ecx,%ecx
	movb	%dh,%cl
	xorb	(%ebx),%dl
	movb	%dl,%al
	xorw	(%ebp,%eax,2),%cx
	incl	%ebx
.L232:
	xorl	%edx,%edx
	movb	%ch,%dl
	xorb	(%ebx),%cl
	movb	%cl,%al
	xorw	(%ebp,%eax,2),%dx
	incl	%ebx
.L230:
	decl	%esi
	jge	.L231
	xorl	%eax,%eax
	cmpb	(%ebx),%dl
	jne	.L234
	incl	%ebx
	cmpb	(%ebx),%dh
	je	.L235
.L234:
	incl	%eax
.L235:
	popl	%ebp
	popl	%ebx
	popl	%esi
	ret	
	.align	16,7,4
	.size	tcrc,.-tcrc
	.ident	"acomp: (CCS) 2.0  07/24/92 "
	.text
	.ident	"optim: (CCS) 2.0  07/24/92 "

#endif	/* AS */

#endif	/* defined(i386) && SYSVREL >= 4 && linux == 0 */

#endif	/* mips */

#endif	/* ns32000 */

#endif	/* mc68000 */

#endif	/* pdp11 */

#endif	/* vax */



#ifdef	AS

#if	M_XENIX == 1 && scounix != 1

	end

#endif	/* M_XENIX == 1 && scounix != 1 */

#else	/* AS */

#ifdef	C_TABLE

Crc_t	crc16t_[256] =
{
	       0, 0140301, 0140601,    0500, 0141401,   01700,   01200, 0141101
	,0143001,   03300,   03600, 0143501,   02400, 0142701, 0142201,   02100
	,0146001,   06300,   06600, 0146501,   07400, 0147701, 0147201,   07100
	,  05000, 0145301, 0145601,   05500, 0144401,   04700,   04200, 0144101
	,0154001,  014300,  014600, 0154501,  015400, 0155701, 0155201,  015100
	, 017000, 0157301, 0157601,  017500, 0156401,  016700,  016200, 0156101
	, 012000, 0152301, 0152601,  012500, 0153401,  013700,  013200, 0153101
	,0151001,  011300,  011600, 0151501,  010400, 0150701, 0150201,  010100
	,0170001,  030300,  030600, 0170501,  031400, 0171701, 0171201,  031100
	, 033000, 0173301, 0173601,  033500, 0172401,  032700,  032200, 0172101
	, 036000, 0176301, 0176601,  036500, 0177401,  037700,  037200, 0177101
	,0175001,  035300,  035600, 0175501,  034400, 0174701, 0174201,  034100
	, 024000, 0164301, 0164601,  024500, 0165401,  025700,  025200, 0165101
	,0167001,  027300,  027600, 0167501,  026400, 0166701, 0166201,  026100
	,0162001,  022300,  022600, 0162501,  023400, 0163701, 0163201,  023100
	, 021000, 0161301, 0161601,  021500, 0160401,  020700,  020200, 0160101
	,0120001,  060300,  060600, 0120501,  061400, 0121701, 0121201,  061100
	, 063000, 0123301, 0123601,  063500, 0122401,  062700,  062200, 0122101
	, 066000, 0126301, 0126601,  066500, 0127401,  067700,  067200, 0127101
	,0125001,  065300,  065600, 0125501,  064400, 0124701, 0124201,  064100
	, 074000, 0134301, 0134601,  074500, 0135401,  075700,  075200, 0135101
	,0137001,  077300,  077600, 0137501,  076400, 0136701, 0136201,  076100
	,0132001,  072300,  072600, 0132501,  073400, 0133701, 0133201,  073100
	, 071000, 0131301, 0131601,  071500, 0130401,  070700,  070200, 0130101
	, 050000, 0110301, 0110601,  050500, 0111401,  051700,  051200, 0111101
	,0113001,  053300,  053600, 0113501,  052400, 0112701, 0112201,  052100
	,0116001,  056300,  056600, 0116501,  057400, 0117701, 0117201,  057100
	, 055000, 0115301, 0115601,  055500, 0114401,  054700,  054200, 0114101
	,0104001,  044300,  044600, 0104501,  045400, 0105701, 0105201,  045100
	, 047000, 0107301, 0107601,  047500, 0106401,  046700,  046200, 0106101
	, 042000, 0102301, 0102601,  042500, 0103401,  043700,  043200, 0103101
	,0101001,  041300,  041600, 0101501,  040400, 0100701, 0100201,  040100
};

#endif	/* C_TABLE */

#ifdef	C_VERSION

/*
**	Accumulate and return CRC.
*/

Crc_t
#ifdef	ANSI_C
acrc(Crc_t crc, register char *buffer, int nbytes)
#else	/* ANSI_C */
acrc(crc, buffer, nbytes)
	Crc_t		crc;
	register char *	buffer;
	int		nbytes;
#endif	/* ANSI_C */
{
	register Uint	tcrc = crc;
	register int	i;
	register int	j;

	if ( (i = (nbytes+7) >> 3) > 0 || nbytes > 0 )
	{
		if ( (j = (nbytes & 7)) != 0 )
			buffer -= 8 - j;

		switch ( j )
		{
		default:
		case 0:	do {	tcrc = crc16t_[(tcrc^(buffer[0]))&0xff] ^ ((tcrc>>8)&0xff);
		case 7:		tcrc = crc16t_[(tcrc^(buffer[1]))&0xff] ^ ((tcrc>>8)&0xff);
		case 6:		tcrc = crc16t_[(tcrc^(buffer[2]))&0xff] ^ ((tcrc>>8)&0xff);
		case 5:		tcrc = crc16t_[(tcrc^(buffer[3]))&0xff] ^ ((tcrc>>8)&0xff);
		case 4:		tcrc = crc16t_[(tcrc^(buffer[4]))&0xff] ^ ((tcrc>>8)&0xff);
		case 3:		tcrc = crc16t_[(tcrc^(buffer[5]))&0xff] ^ ((tcrc>>8)&0xff);
		case 2:		tcrc = crc16t_[(tcrc^(buffer[6]))&0xff] ^ ((tcrc>>8)&0xff);
		case 1:		tcrc = crc16t_[(tcrc^(buffer[7]))&0xff] ^ ((tcrc>>8)&0xff);
				buffer += 8;
			} while ( --i );
		}
	}

	return (Crc_t)tcrc;
}



/*
**	Test CRC, and if error, return true.
**	Store correct CRC at end of string.
*/

bool
crc(buffer, nbytes)
	register char *	buffer;
	int		nbytes;
{
	register Uint	tcrc;
	register bool	i;

	tcrc = acrc((Crc_t)0, buffer, nbytes);
	buffer += nbytes;

	if ( LOCRC(tcrc) != BYTE(*buffer) )
		i = true;
	else
		i = false;
	*buffer++ = LOCRC(tcrc);

	if ( HICRC(tcrc) != BYTE(*buffer) )
		i = true;
	*buffer++ = HICRC(tcrc);

	return i;
}



/*
**	Test CRC, and if error, return true.
*/

bool
tcrc(buffer, nbytes)
	register char *	buffer;
	int		nbytes;
{
	register Uint	crct;

	crct = acrc((Crc_t)0, buffer, nbytes);
	buffer += nbytes;

	if ( LOCRC(crct) != BYTE(*buffer++) )
		return true;

	if ( HICRC(crct) != BYTE(*buffer) )
		return true;

	return false;
}

#endif	/* C_VERSION */

#endif	/* AS */
