#!/bin/bash

CAPTUREDIR="" #fill this in with the path used in config.json, no ending /

# Creates a list of radio IDs in a file named "radiolist.csv" located in the
# shortName directory along side the recordings, and logs radio activity to
# "radiolog.csv" files located in each day's recordings directory

# file format: radioID,timestamp,action,talkgroup,
# for radiolist.csv, acknowledgment response timestamps are added at the end
# when they are seen after a different action

# Feel free to customize this script; to use for multiple systems, include in
# each system's config.json section

# NOTE: You need to run "echo > radiolist.csv" where the file(s) is going to be
# beforehand as sed doesn't work on empty files, and to capture actions before
# trunk-recorder makes the daily directory upon recording the first call, set
# up a cron task of: 0 0 * * * mkdir -p <capturedir>/$(date +\%Y/\%-m/\%-d/)

# sed usage based on https://stackoverflow.com/a/49852337

printf -v TRDATE '%(%Y/%-m/%-d)T'
printf -v NOWTIME '%(%s)T'
if [ ! "$3" == "ackresp" ]; then
  sed -i -e '/^'${2}',.*/{s//'${2}','${NOWTIME}','${3}','${4}',/;:a;n;ba;q}' -e '$a'${2}','${NOWTIME}','${3}','${4}',' $CAPTUREDIR/$1/radiolist.csv
else
  sed -i -e '/^'${2}',\([0-9]*\),ackresp,,/{s//'${2}','${NOWTIME}',\1,ackresp,,/;:a;n;ba;q}' -e '/^'${2}',\([0-9]*\),\([a-z]*\),\([0-9]*\),\([0-9]*\)/{s//'${2}',\1,\2,\3,'${NOWTIME}'/;:a;n;ba;q}' -e '$a'${2}','${NOWTIME}',ackresp,,' $CAPTUREDIR/$1/radiolist.csv
fi
echo "$2,$NOWTIME,$3,$4" >> $CAPTUREDIR/$1/$TRDATE/radiolog.csv
