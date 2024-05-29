BEGIN	{
		filter = ""
		flags = ""
		intermittent = 0
		restrict = ""
	}

/remote address/	{ address = $2; }
/link filter/		{ filter = $2; }
/link flags/		{ flags = $2; }
/restrict/		{ restrict = $2; }
/^address/		{ myname = $2; }
/type:.*dial/		{ intermittent = 1; }

END	{
		printf "\nadd	%s\n", address;
		printf "link	%s,\n", myname;
		printf "	%s\n", address;
		if ( flags != "" ) {
			printf "	%s\n", flags;
		}
		printf "restrict	%s,\n", address;
		printf "	%s\n", myname;
		if ( restrict != "" ) {
			printf "	%s\n", restrict;
		}
		printf "filter	%s\n", address;
		if ( filter != "" ) {
			if ( filter !~ /\// ) {
				printf "	_lib/%s\n", filter;
			} else {
				printf "	%s\n", filter;
			}
		}
		if ( intermittent == 1 ) {
			printf "delay	%s,\n", address;
			printf "	%s\n", myname;
			printf "	86400\n";
		}
	}
