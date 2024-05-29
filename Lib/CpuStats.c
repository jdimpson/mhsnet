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


#define	NEED_HZ
#define	TIMES
#define	STDIO

#include	"global.h"

#if	BSD4 > 1 || (BSD4 == 1 && BSD4V == 'c')

#include	<sys/time.h>
#include	<sys/resource.h>



/*
**	Print out system cpu stats.
*/

void
CpuStats(fd, elapsed)
	FILE *		fd;
	Time_t		elapsed;
{
	Ulong		dtot;
	Ulong		ctot;
	char		tbuf[TIMESTRLEN];
	struct rusage	dus;
	struct rusage	cus;
	static char	hdr[] =	"Usages:    user + sys = tot CPU%%   io(i+o)      pf(min+maj)      sigs   cs(iv+v)\n";
	/*
	**			"Children: NNNNN NNNNN NNNNN NNN% NNNNNN+NNNNNN NNNNNNN+NNNNNN NNNNNNN NNNNNNN+NNNNNNN\n"
	*/
	static char	fmt[] =	"%-9s %5lu %5lu %5lu %3lu%% %6lu+%-6lu %7lu+%-6lu %7lu %7lu+%lu\n";

	(void)getrusage(RUSAGE_SELF, &dus);
	(void)getrusage(RUSAGE_CHILDREN, &cus);

	(void)fprintf(fd, "Elapsed time: %s\n", TimeString((Ulong)elapsed, tbuf));

	if ( elapsed == 0 )
		elapsed = 1;

	(void)fprintf(fd, hdr);

	dtot = ((dus.ru_utime.tv_sec + dus.ru_stime.tv_sec) * 10 + 5 +
		  ((dus.ru_utime.tv_usec + dus.ru_stime.tv_usec) / 100000)) /10;

	(void)fprintf
	(
		fd, fmt,
		"Daemon:",
		(PUlong)(dus.ru_utime.tv_sec + (dus.ru_utime.tv_usec >= 500000 ? 1 : 0)),
		(PUlong)(dus.ru_stime.tv_sec + (dus.ru_stime.tv_usec >= 500000 ? 1 : 0)),
		(PUlong)dtot,
		(PUlong)(dtot * 100 / elapsed),
		(PUlong)dus.ru_inblock,
		(PUlong)dus.ru_oublock,
		(PUlong)dus.ru_minflt,
		(PUlong)dus.ru_majflt,
		(PUlong)dus.ru_nsignals,
		(PUlong)dus.ru_nivcsw,
		(PUlong)dus.ru_nvcsw
	);

	ctot = ((cus.ru_utime.tv_sec + cus.ru_stime.tv_sec) * 10 + 5 +
		  ((cus.ru_utime.tv_usec + cus.ru_stime.tv_usec) / 100000)) /10;

	(void)fprintf
	(
		fd, fmt,
		"Children:",
		(PUlong)(cus.ru_utime.tv_sec + (cus.ru_utime.tv_usec >= 500000 ? 1 : 0)),
		(PUlong)(cus.ru_stime.tv_sec + (cus.ru_stime.tv_usec >= 500000 ? 1 : 0)),
		(PUlong)ctot,
		(PUlong)(ctot * 100 / elapsed),
		(PUlong)cus.ru_inblock,
		(PUlong)cus.ru_oublock,
		(PUlong)cus.ru_minflt,
		(PUlong)cus.ru_majflt,
		(PUlong)cus.ru_nsignals,
		(PUlong)cus.ru_nivcsw,
		(PUlong)cus.ru_nvcsw
	);

	(void)fprintf
	(
		fd, fmt,
		"Totals:",
			/* rounding here is sloppy but close */
		(PUlong)(((cus.ru_utime.tv_sec + dus.ru_utime.tv_sec) * 10 + 5 +
		  ((cus.ru_utime.tv_usec + dus.ru_utime.tv_usec) / 100000)) /10),
		(PUlong)(((cus.ru_stime.tv_sec + dus.ru_stime.tv_sec) * 10 + 5 +
		  ((cus.ru_stime.tv_usec + dus.ru_stime.tv_usec) / 100000)) /10),
		(PUlong)(dtot+ctot),
		(PUlong)((dtot+ctot) * 100 / elapsed),
		(PUlong)(cus.ru_inblock + dus.ru_inblock),
		(PUlong)(cus.ru_oublock + dus.ru_oublock),
		(PUlong)(cus.ru_minflt + dus.ru_minflt),
		(PUlong)(cus.ru_majflt + dus.ru_majflt),
		(PUlong)(cus.ru_nsignals + dus.ru_nsignals),
		(PUlong)(cus.ru_nivcsw + dus.ru_nivcsw),
		(PUlong)(cus.ru_nvcsw + dus.ru_nvcsw)
	);
}

#else	/* BSD4 ... */

/*
**	Print out system cpu times.
*/

void
CpuStats(fd, elapsed)
	FILE *		fd;
	Time_t		elapsed;
{
	register Ulong	dtot;
	register Ulong	ctot;
	register Ulong	pcnt;

	struct tms	tbuf;
	char		tsbuf1[TIMESTRLEN];
	char		tsbuf2[TIMESTRLEN];
	char		tsbuf3[TIMESTRLEN];
	static char	hdr[] = "Usages:      user  +  sys = total CPU%%\n";
	/*
	**			"Children: NNNNNNN NNNNNNN NNNNNNN NNN%\n"
	*/
	static char	fmt[] = "%-9s %7s %7s %7s";
	static char	cfmt1[] = " %3lu%%\n";
	static char	cfmt2[] = " %3.1f%%\n";

	(void)times(&tbuf);

	(void)fprintf(fd, "Elapsed time: %s\n", TimeString((Ulong)elapsed, tsbuf1));

	if ( elapsed == 0 )
		elapsed = 1;

	(void)fprintf(fd, "%s", hdr);

	dtot = (tbuf.tms_utime + tbuf.tms_stime + HZ-1)/HZ;

	(void)fprintf
	(
		fd, fmt, "Daemon:",
		TimeString((Ulong)(tbuf.tms_utime + HZ-1)/HZ, tsbuf1),
		TimeString((Ulong)(tbuf.tms_stime + HZ-1)/HZ, tsbuf2),
		TimeString((Ulong)dtot, tsbuf3)
	);

	if ( (pcnt = dtot * 100 / elapsed) < 10 )
		(void)fprintf(fd, cfmt2, dtot*100.0/(float)elapsed);
	else
		(void)fprintf(fd, cfmt1, (PUlong)pcnt);

	ctot = (tbuf.tms_cutime + tbuf.tms_cstime + HZ-1)/HZ;

	(void)fprintf
	(
		fd, fmt, "Children:",
		TimeString((Ulong)(tbuf.tms_cutime + HZ-1)/HZ, tsbuf1),
		TimeString((Ulong)(tbuf.tms_cstime + HZ-1)/HZ, tsbuf2),
		TimeString((Ulong)ctot, tsbuf3)
	);

	if ( (pcnt = ctot * 100 / elapsed) < 10 )
		(void)fprintf(fd, cfmt2, ctot*100.0/(float)elapsed);
	else
		(void)fprintf(fd, cfmt1, (PUlong)pcnt);

	(void)fprintf
	(
		fd, fmt, "Totals:",
		TimeString((Ulong)(tbuf.tms_utime + tbuf.tms_cutime + HZ-1)/HZ, tsbuf1),
		TimeString((Ulong)(tbuf.tms_stime + tbuf.tms_cstime + HZ-1)/HZ, tsbuf2),
		TimeString((Ulong)dtot+ctot, tsbuf3)
	);

	ctot += dtot;

	if ( (pcnt = ctot * 100 / elapsed) < 10 )
		(void)fprintf(fd, cfmt2, ctot*100.0/(float)elapsed);
	else
		(void)fprintf(fd, cfmt1, (PUlong)pcnt);
}
#endif	/* BSD4 ... */
