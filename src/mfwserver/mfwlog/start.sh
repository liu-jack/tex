#!/bin/bash

ulimit -c unlimited
old_path=`pwd`
cd `dirname $0`
path=`pwd`
server=mfwlog

$path/stop.sh

$path/$server --config=$path/$server.mfw.conf &

sleep 1
if [ -n "`ps -ef|grep "$path/$server"|grep -v "grep"`" ]
then
	echo "start $path/$server ok ...."
else
	echo "start $path/$server failed ...."
fi


cd $old_path 1>/dev/null
