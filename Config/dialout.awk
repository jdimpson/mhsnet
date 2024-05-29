BEGIN		{ FS="\t"; cooked="no"; annex="" }

/modem type/	{ modem = $2; next; }
/modem device/	{ modemdev = $2; next; }
/login name/	{ loginname = $2; next; }
/password/	{ password = $2; next; }
/phone number2/	{ telno2 = $2; next; }
/phone number/	{ telno = $2; next; }
/line cookedX/	{ cooked = "-CX"; next; }
/line cooked/	{ cooked = "-c"; next; }
/line speed/	{ linespeed = $2; next; }
/line fixspeed/	{ linespeed = $2; fix = "true"; next; }
/link filter/	{ filter = $2; next; }
/link flags/	{ flags = $2; next; }
/annexhost/	{ annex = $2; next; }
/annexuser/	{ auser = $2; next; }
/annexpass/	{ apass = $2; next; }

END		{
			printf "modem='%s'\n", modem;
			printf "modemdev='%s'\n", modemdev;
			printf "loginname='%s'\n", loginname;
			printf "password='%s'\n", password;
			printf "telno='%s'\n", telno;
			if ( telno2 != "" )
				printf "telno2='%s'\n", telno2;
			if ( fix != "" )
				printf "fixspeed='%s'\n", linespeed;
			else
				printf "linespeed='%s'\n", linespeed;
			printf "filter='%s'\n", filter;
			printf "flags='%s'\n", flags;
			printf "cooked='%s'\n", cooked;
			if ( annex != "" )
				printf "annexuser='%s'\nannexpass='%s'\nannexhost='%s'\n", auser, apass, annex
		}
