BEGIN		{ FS="\t"; }

/modem device/	{ modemdev = $2; }
/login name/	{ loginname = $2; }
/password/	{ password = $2; }
/line speed/	{ linespeed = $2; }
/link filter/	{ filter = $2; }
/link flags/	{ flags = $2; next; }

END		{
			printf "modemdev='%s'\n", modemdev;
			printf "loginname='%s'\n", loginname;
			printf "password='%s'\n", password;
			printf "linespeed='%s'\n", linespeed;
			printf "filter='%s'\n", filter;
			printf "flags='%s'\n", flags;
		}
