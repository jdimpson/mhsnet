#!/usr/bin/env python
#
#	SCCSID	@(#)httpget.py	@(#)httpget.py	1.9 05/12/16
#
# Simple wrapper around urrlib.
#
# Fetches URL given as first argument
# saves as either second argument, or 
# as first argument (after last '/').

import os, signal, string, sys, urllib

proxies = (
#	('http_proxy', 'http://www-cache.cs.usyd.edu.au:8000'),
#	('ftp_proxy', 'http://www-cache.cs.usyd.edu.au:8000'),
)


def args():
	if not (len(sys.argv)) in [2,3]:
		sys.stderr.write("Usage: %s url [filename]\n" % os.path.split(sys.argv[0])[1])
		sys.exit(1)

	addr = sys.argv[1]
	try:
		fname = sys.argv[2]
	except:
		n = string.rfind(addr, '/', 0)
		if n < 0:
			fname = addr[:]
		else:
			fname = addr[n+1:]

	if addr == 'http://xxx.error.yyy/':
		sys.stderr.write("debugging error exit on %s\n" % addr)
		sys.exit(2)

	try:
		ufp = urllib.urlopen(addr)
	except:
		syserror("access", addr)

	try:
		lfp = open(fname, 'w')
	except:
		syserror("create", fname)

	return ufp, lfp, addr, fname


def catch(sig, frm):
	sys.stderr.write("Caught signal %s.\n" % sig)
	os._exit(3)	# Avoid all exception handlers!


def copy(ufp, lfp, addr, fname):
	try:
		data = ufp.read(8192)
	except:
		syserror("read", addr)

	while data:
		try:
			lfp.write(data)
		except:
			syserror("write", fname)
		try:
			data = ufp.read(8192)
		except:
			syserror("read", addr)


def startup():
	for name, value in proxies:
		if not os.environ.get(name):
			os.environ[name] = value

	signal.signal(signal.SIGTERM, catch)
	signal.signal(signal.SIGHUP, catch)
	signal.signal(signal.SIGALRM, catch)


def syserror(type, file):
	sys.stderr.write("Can't %s \"%s\"\n%s\n" % (type, file, sys.exc_info()[1]))
	sys.exit(2)


def main():
	startup()
	apply(copy, args())
	del urllib._urlopener	# workaround for urllib __del__() bug
	sys.exit(0)


if __name__ == '__main__':
	main()
