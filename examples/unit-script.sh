#!/bin/bash

#README: unit-script.txt

CAPTUREDIR=""

printf -v TRDATE '%(%Y/%-m/%-d)T'
printf -v NOWTIME '%(%s)T'
if [ ! "$3" == "ackresp" ]; then
  sed -i -e '/^'${2}',.*/{s//'${2}','${NOWTIME}','${3}','${4}',/;:a;n;ba;q}' -e '$a'${2}','${NOWTIME}','${3}','${4}',' $CAPTUREDIR/$1/radiolist.csv
else
  sed -i -e '/^'${2}',\([0-9]*\),ackresp,,/{s//'${2}','${NOWTIME}',ackresp,,/;:a;n;ba;q}' -e '/^'${2}',\([0-9]*\),\([a-z]*\),\([0-9]*\),\([0-9]*\)/{s//'${2}',\1,\2,\3,'${NOWTIME}'/;:a;n;ba;q}' -e '$a'${2}','${NOWTIME}',ackresp,,' $CAPTUREDIR/$1/radiolist.csv
fi
echo "$2,$NOWTIME,$3,$4" >> $CAPTUREDIR/$1/$TRDATE/radiolog.csv
