#!/bin/sh
#

# PROVIDE: local_mhsnet
# REQUIRE: mail

. /etc/rc.subr

name="local_mhsnet"
rcvar=$name

SPOOLDIR=/var/spool/MHSnet
eval `grep SPOOLDIR= /usr/lib/MHSnetparams 2>/dev/null`

extra_commands="reload"
command="netinit"

start_cmd="${SPOOLDIR}/_lib/start"
stop_cmd="${SPOOLDIR}/_bin/netcontrol shutdown"
status_cmd="${SPOOLDIR}/_bin/netcontrol status"
reload_cmd="${SPOOLDIR}/_bin/netcontrol scan"

required_files="${SPOOLDIR}/_lib/start"

load_rc_config $name
run_rc_command "$1"
