#!/bin/bash
printf -v TRDATE '%(%Y/%-m/%-d)T'
printf -v NOWTIME '%(%s)T'
SHORTNAME=$1
RADIOID=$2
ACTION=$3
TALKGROUP=$4
if $3 != "ackresp"
sed <capture_dir>/$1/radiolist.csv 's/^$RADIOID,(.*)/$RADIOID,$NOWTIME,$ACTION,$TALKGROUP,/'
elif
sed <capture_dir>/$1/radiolist.csv 's/$RADIOID,(\d+),([on|join|off|call]),(\d+),(\d+)/$RADIOID,$1,$2,$3,$NOWTIME'
done
echo "$2,$3,$4" >> <capture_dir>/$1/$TRDATE/radiolog.csv