#!/bin/sh
#
#	@(#)AddSun3Host.sh	1.1 89/04/03
#
#	Convert Sun3 hosts list into commands for Sun4
#

acsstate -VZ $* | awk	'
	BEGIN				{
			hier = ""
			node = ""
			nodecount = 0
			linkcount = 0
			types[0] = "C"
			types[1] = "A"
			types[2] = "P"
			types[3] = "O"
			types[4] = "D"
			types[5] = "E"
			types[6] = "F"
			types[7] = "G"
			types[8] = "H"
			typemaps[1] = "D\tOU1"
			typemaps[2] = "E\tOU2"
			typemaps[3] = "F\tOU3"
			typemaps[4] = "G\tOU4"
			typemaps[5] = "H\tOU5"
			unmaps[1] = "D"
			unmaps[2] = "E"
			unmaps[3] = "F"
			unmaps[4] = "G"
			unmaps[5] = "H"
		}
	/^Local domain hierarchy/	{
			next
		}
	/^ACSnet map/			{
			next
		}
	/^   +/				{
			next
		}
	/^ -> /				{
			if ( $3 ~ /msg/ ) {
				links[linkcount] = "N=" $2 "|" node
				if ( $3 ~ /dead/ )
					linkflags[linkcount] = "dead"
				if ( $3 ~ /int/ )
					linkdelays[linkcount] = "86400"
				for ( i = split($0, a) ; i > 4 ; i-- )
					if ( a[i] ~ /cost\/Mb/ )
						linkcosts[linkcount] = a[i-1]
				linkcount++
			}
			next
		}
	/^[^ ]+/			{
			node = "N=" $1
			nodes[nodecount++] = node
			hier = $2
			n = index(hier, "{")
			l = length(hier)
			if ( n > 0 && l > (1+n) ) {
				hier = substr(hier, n+1, l-(1+n))
				if ( (n = split(hier, a, ".")) == 0 ) {
					a[1] = hier
					n = 1
				}
				hier = ""
				skipmd = 0
				if ( a[n] == "oz" )
					j = n+1
				else
				if ( n == 1 )
					j = 0
				else {
					skipmd = 1
					j = n
				}
				for ( i = 1 ; i <= n ; i++ ) {
					hier = hier "." types[j] "=" a[i]
					j--
					if ( skipmd && j == 1 ) j = 0
				}
				if ( a[n] == "oz" )
					hier = hier ".C=au"
				doms = $3
			}
			else {
				hier = ".P=oz.C=au"
				doms = $2
			}
			n = index(doms, "[")
			l = length(doms)
			if ( n > 0 && l > (1+n) ) {
				doms = substr(doms, n+1, l-(1+n))
				if ( split(doms, a, "|") == 0 ) {
					a[1] = doms
				}
				if ( a[1] != "oz" && index(a[1], "(") == 0 ) {
					if ( hier == ".P=oz.C=au" )
						hier = ".O=" a[1] hier
					if ( n = index(hier, a[1]) )
						visible[node] = substr(hier, n-2, 1000)
					else
						visible[node] = "P=oz.C=au"
				}
				else
					visible[node] = "P=oz.C=au"
			}
			else
				visible[node] = "P=oz.C=au"
			hierarchy[node] = hier
			next
		}
	/^ +"/				{
			next
		}
	END				{
			for ( i in typemaps ) {
				printf "0map\t%s\n", typemaps[i]
			}
			for ( i in nodes ) {
				node = nodes[i]
				printf "1add\t%s%s\n", node, hierarchy[node]
				printf "1ialias\t%s\t%s%s\n", node, node, hierarchy[node]
				if ( visible[node] != "" )
					printf "2visible\t%s%s\t%s\n", node, hierarchy[node], visible[node]
			}
			for ( j in links ) {
				link = links[j]
				k = index(link, "|" )
				node = substr(link, k+1)
				link = substr(link, 1, k-1)
				printf "3halflink\t%s%s,%s%s\n", node, hierarchy[node], link, hierarchy[link]
				if ( linkflags[j] != "" )
					printf "4flag\t%s%s,%s%s\t%s\n", node, hierarchy[node], link, hierarchy[link], linkflags[j]
				if ( linkcosts[j] != "" )
					printf "5cost\t%s%s,%s%s\t%s\n", node, hierarchy[node], link, hierarchy[link], linkcosts[j]
				if ( linkdelays[j] != "" )
					printf "6delay\t%s%s,%s%s\t%s\n", node, hierarchy[node], link, hierarchy[link], linkdelays[j]
			}
			for ( i in unmaps ) {
				printf "9map\t%s\n", unmaps[i]
			}
		}' | sort | sed 's/.//'
