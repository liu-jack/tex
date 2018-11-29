#!/bin/bash
# 打包服务器二进制文件并上传到CC
if [ $# -ne 1 ] ;then
	echo "Usage: $0  env(\"d\")"
	exit -1
fi

source config.sh
get_env_config $1
source remote_cmd.sh

runcmd $rel_user@$cc_ip "mkdir -p $version_backup"

for server in ${all_svr[@]} ; do

	./pack_server.sh $server

	pack_name=$app-$server-`date +%Y%m%d`.tar.gz
	putfile $rel_user@$cc_ip $pack_name $version_backup  
	rm $pack_name
done

echo -e ">>> $0 success\n"
