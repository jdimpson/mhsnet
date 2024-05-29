/remote address/	{ address = $2; }
/^address/		{ myname = $2; }

END	{
		if ( myname != address ) {
			printf "\nbreak	%s,\n", myname;
			printf "	%s\n", address;
		}
	}
