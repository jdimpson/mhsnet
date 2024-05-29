#!/bin/sh
# SYSTEM V ps listing sorted by current CPU use
ps -el|sort +0.22 -0.25|egrep -v defunct|tail -23
