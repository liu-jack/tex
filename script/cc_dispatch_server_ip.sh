#!/bin/bash
# 从CC分发服务器到各个agent

if [[ $# -ne 2 ]]; then
	echo "Usage: $0 pack_name dst_ip"
	exit -1
fi
source config.sh
source remote_cmd.sh

pack_name=$1
dst_ip=$2
server=`echo $pack_name |cut -d "-" -f2`
if [ ! -f $version_backup/$pack_name ]; then
	echo "cannot find file: $version_backup/$pack_name"
	exit -1
fi

echo ">>> copy $pack_name to $dst_ip:$version_backup"
runcmd $rel_user@$dst_ip "mkdir -p $version_backup"
putfile $rel_user@$dst_ip $version_backup/$pack_name $version_backup

echo ">>> $0 success"
