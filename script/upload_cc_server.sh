#!/bin/bash
# 打包服务器二进制文件并上传到CC

source config.sh
if [ $# -ne 2 ] ;then
	echo "upload pack server to $app-server-date-svn.tar.gz"
	echo "Usage: $0 env server_name"
	exit -1
fi
get_env_config $1
source remote_cmd.sh

server=$2

./pack_server.sh $server || exit 9

pack_name=$app-$server-`date +%Y%m%d`.tar.gz

echo ">>> copy $pack_name to $cc_ip:$version_backup"
runcmd $rel_user@$cc_ip "mkdir -p $version_backup"
putfile $rel_user@$cc_ip $pack_name $version_backup
rm $pack_name

echo ">>> $0 success"

