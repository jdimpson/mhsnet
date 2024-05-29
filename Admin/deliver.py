#!/usr/bin/env python
#
#	SCCSID	@(#)deliver.py	1.1 02/05/21
#
"""
	Deliver a mail message to a list of local login names
		via the local SMTP receiver.

	Optionally specify a from address.

	Can be used in place of /usr/lib/sendmail if some other program
	is delivering local mail and an SMTP receiver daemon is active.
"""

Usage = """Usage: %s [--debug] [-o<sendmail_opt> ...] \\
	[-f<from_addr>] <login_name> ...\n"""

import getopt, os, signal, smtplib, sys

DfltSMTPhost = 'localhost'
DfltSMTPsender = 'postmaster'
Debug = 0


def usage(reason=''):
	sys.stdout.flush()
	if reason: sys.stderr.write('\t%s\n\n' % reason)
	head, tail = os.path.split(sys.argv[0])
	sys.stderr.write(Usage % tail)
	sys.stderr.write(__doc__)
	sys.exit(1)


def args():
	try:
		optlist, args = getopt.getopt(sys.argv[1:], 'f:o:?', ['debug', 'help'])
	except getopt.error, val:
		usage(val)

	froma = DfltSMTPsender

	for opt,val in optlist:
		if opt == '--debug':
			global Debug; Debug = 1
		elif opt == '-f':
			froma = val
		elif opt == '-o':
			pass	# Ignore sendmail options
		else:
			usage()

	if len(args) < 1:
		usage()

	toa = args

	data = sys.stdin.read()

	return froma, toa, data


def send(froma, toa, data):

	try:
		s = smtplib.SMTP(DfltSMTPhost)
		if Debug: s.set_debuglevel(Debug)
		senderrs = s.sendmail(froma, toa, data)
		s.quit()
	except smtplib.SMTPSenderRefused, val:
		sys.stderr.write('SMTP refused sender "%s" [%s]\n' % (val.sender, val.smtp_error))
		sys.exit(1)
	except smtplib.SMTPRecipientsRefused, val:
		senderrs = val.recipients
	except smtplib.SMTPException, val:
		sys.stderr.write('SMTP error: %s\n' % str(val))
		sys.exit(1)

	if not senderrs:
		return

	for a,(c,r) in senderrs.items():
		sys.stderr.write('recipient "%s" refused: %s\n' % (a, r))
	sys.exit(1)


def catch(sig, frm):
	sys.stderr.write("Caught signal %s.\n" % sig)
	os._exit(3)	# Avoid all exception handlers!


def startup():
	signal.signal(signal.SIGTERM, catch)
	signal.signal(signal.SIGHUP, catch)
	signal.signal(signal.SIGALRM, catch)


def main():
	startup()
	apply(send, args())
	sys.exit(0)


if __name__ == '__main__':
	main()
