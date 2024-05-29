BEGIN		{ SUN3 = ""; SUN3A = ""; annex = ""; cook = ""; fix = ""; perm = ""; reliable = ""; }

/annexhost/	{ annex = $2; next; }
/annexuser/	{ auser = $2; next; }
/annexpass/	{ apass = $2; next; }
/crontime/	{ crontime = $2; next; }
/dialout/	{ dialout = 1; next; }
/directout/	{ directout = 1; next; }
/filter43/	{ SUN3 = "3"; SUN3A = " -3"; next; }
/line cookedX/	{ cook = " -CX"; next; }
/line cooked/	{ cook = " -c"; next; }
/line speed/	{ linespeed = $2; next; }
/line fixspeed/	{ linespeed = $2; fix = " -e"; next; }
/link address/	{ linka = $2; next; }
/login name/	{ loginstr = $2; next; }
/modem device/	{ device = $2; next; }
/modem type/	{ modem = $2; next; }
/password/	{ passwdstr = $2; next; }
/phone number2/	{ dialstr2 = $2; next; }
/phone number/	{ dialstr = $2; next; }
/reliable/	{ reliable = " -r"; next; }
/remote address/{ linkta = $2; next; }
/remote alias/	{ alias = $2; next; }
/tcpin/		{ tcpin = 1; next; }
/tcpout/	{ tcpout = 1; next; }
/udpout/	{ udpout = 1; next; }
/x25out/	{ x25out = $2; next; }
/x25address/	{ x25address = $2; next; }
/circuit/	{ circuit = $2; next; }
/address2/	{ address2 = $2; next; }
/address3/	{ address3 = $2; next; }
/address/	{ address = $2; next; }
/cmx_local/	{ cmxlocal = $2; next; }
/cmx_remote/	{ cmxremote = $2; next; }
/xtiin/		{ xtiin = $2; next; }
/xtiout/	{ xtiout = $2; next; }

END	{
		if ( crontime ~ /restart/ )
		{
			crontime = "restart";
			perm = " -p";
		}
		if ( crontime ~ /runonce/ )
			crontime = "runonce";
		if ( dialout == 1 )
		{
			printf "call-%s	", linka;
			printf "%s _call/call.modem_0 -f%s%s%s%s\n", crontime, SUN3A, cook, fix, perm;
			if ( annex != "" )
			{
				if ( auser != "" )
					printf "\t-D aloginstr=%s\n", auser
				if ( apass != "" )
					printf "\t\"-D apasswdstr=%s\"\n", apass
				printf "\t-D ahost=%s -S hayes_annex.cs\n", annex
			}
			printf "\t%s\n", linkta;
			printf "\t\"%s@%s\"\n", modem, device;
			printf "\t\"%s@%s", dialstr, linespeed;
			if ( dialstr2 != "" )
				printf "|%s@%s\"\n", dialstr2, linespeed;
			else
				printf "\"\n";
			printf "\t%s\n", loginstr;
			printf "\t\"%s\"\n", passwdstr;
		}
		if ( directout == 1 )
		{
			printf "call-%s	", linka;
			printf "%s _call/call.login%s -f\n", crontime, SUN3;
			printf "\t%s\n", linkta;
			printf "\t%s\n", device;
			printf "\t%s\n", linespeed;
			printf "\t%s\n", loginstr;
			printf "\t\"%s\"\n", passwdstr;
		}
		if ( tcpin == 1 )
		{
			printf "tcplisten restart _lib/tcplisten -f\n";
		}
		if ( tcpout == 1 )
		{
			printf "tcp-%s restart _call/call.tcp -f%s\n", linka, reliable;
			if ( reliable != "" )
				printf "\t-d HTdaemon\n";
			if ( passwdstr != "" )
				printf "\t-p \"%s\"\n", passwdstr;
			printf "\t%s\n", linkta;
			if ( alias != "" && alias != linkta )
				printf "\t%s\n", alias;
		}
		if ( udpout == 1 )
		{
			printf "udp-%s restart _call/call.udp%s -f\n", linka, SUN3;
			if ( passwdstr != "" )
				printf "\t-p \"%s\"\n", passwdstr;
			printf "\t%s\n", linkta;
			if ( alias != "" && alias != linkta )
				printf "\t%s\n", alias;
		}
		if ( x25out == 1 )
		{
			printf "x25-%s ", linka;
			printf "restart _call/call.x25 -f %s\n", linkta;
			printf "\t%s\n", x25address;
		}
		if ( xtiout == 1 )
		{
			printf "xti-%s ", linka;
			printf "restart _call/call.xti -f %s\n", linkta;
			printf "\t%s\n\t%s\n", cmxlocal, cmxremote;
			printf "\t%s\n\t%s\n", address, circuit;
			if ( address2 != "" )
			{
				printf "\t%s\n", address2;
				if ( address3 != "" )
					printf "\t%s\n", address3;
			}
		}
		printf "#\n";
	}
