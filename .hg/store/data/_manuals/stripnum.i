         \   [        ���������T�2ޚ=�'X$l)J��4�            u#!/bin/sh
cd /usr/spool/MHSnet/_man
for i in */*
do
	mv $i `expr $i : '\(.*\).[0-9]'`
done
