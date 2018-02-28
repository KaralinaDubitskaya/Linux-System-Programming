#!/bin/bash
ERR="/tmp/err.log"

files=`find $(readlink -e "$1") -type f -printf "%p %s %M\n" 2>$ERR`

ST_IFS=$IFS
IFS=$'\n'

count=0
for file in $files
do
	let count=count+1
	echo $file
done
echo "$count"

sed -i "s/^/$(basename $0:" ")/g" $ERR
cat $ERR>&2

#rm $ERR

IFS=$ST_IFS

