/*
**	Exit status codes for system programs.
**
**	SCCSID	@(#)sysexits.h	1.2 96/02/09
*/

#undef	EX_OK			/* some unixs' <fcntl.h> uses this instead of X_OK */
#define EX_OK		0	/* successful termination */

#define EX__BASE	64	/* base value for error messages */

#define EX_USAGE	(EX__BASE+0)	/* command line usage error */
#define EX_DATAERR	(EX__BASE+1)	/* data format error */
#define EX_NOINPUT	(EX__BASE+2)	/* cannot open input */
#define EX_NOUSER	(EX__BASE+3)	/* addressee unknown */
#define EX_NOHOST	(EX__BASE+4)	/* host name unknown */
#define EX_UNAVAILABLE	(EX__BASE+5)	/* service unavailable */
#define EX_SOFTWARE	(EX__BASE+6)	/* internal software error */
#define EX_OSERR	(EX__BASE+7)	/* system error (e.g., can't fork) */
#define EX_OSFILE	(EX__BASE+8)	/* critical OS file missing */
#define EX_CANTCREAT	(EX__BASE+9)	/* can't create (user) output file */
#define EX_IOERR	(EX__BASE+10)	/* input/output error */
#define EX_TEMPFAIL	(EX__BASE+11)	/* temp failure; user is invited to retry */
#define EX_PROTOCOL	(EX__BASE+12)	/* remote error in protocol */
#define EX_NOPERM	(EX__BASE+13)	/* permission denied */

#define EX_NET_BASE	(EX__BASE+40)	/* base value for network filter returns */

#define	EX_NEWMESG	(EX_NET_BASE+0)	/* message has been updated */
#define	EX_DROPMESG	(EX_NET_BASE+1)	/* message should be dropped */
#define	EX_RETMESG	(EX_NET_BASE+2)	/* message should be returned */
#define	EX_EXMESG	(EX_NET_BASE+3)	/* message shouldn't be delivered locally */

#define	EX_HANGUP	(EX_NET_BASE+5)	/* vc died */
#define	EX_MAXRT	(EX_NET_BASE+7)	/* max runtime exceeded */
#define	EX_MISMATCH	(EX_NET_BASE+8)	/* link name mismatch */
#define	EX_RDTIMO	(EX_NET_BASE+9)	/* reader timed out */
#define	EX_REMSLOW	(EX_NET_BASE+10)/* vc slow */
#define	EX_REMTERM	EX_OK		/* remote terminate */
#define	EX_SIGPIPE	(EX_NET_BASE+11)/* pipe error */
#define	EX_UNXSIG	(EX_NET_BASE+12)/* unexpected signal */
#define	EX_VCECHO	(EX_NET_BASE+13)/* vc echo */

#define	EX_FDERR	(EX_NET_BASE+20)/* could not open a fd for exec'd proc */
