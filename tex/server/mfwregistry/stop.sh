#!/bin/bash

server=mfwregistry

old_path=`pwd`
cd `dirname $0`
path=`pwd`
killall $path/$server 2>/dev/null

until [ -z "`ps -ef|grep "$path/$server"|grep -v "grep"`" ]
do
	sleep 1
done

echo "stop $path/$server ok ...."


cd $old_path 1>/dev/null
