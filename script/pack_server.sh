#!/bin/bash
# 服务器二进制文件打包

source config.sh
if [ $# -ne 1 ] ;then
	echo "pack server to $app-server-date.tar.gz"
	echo "Usage: $0 server_name"
	exit -1
fi

# 变量定义
temp_pack_path=pack_tmp
server=$1

if [ ! -d $build_path/$server ]; then
	echo "cannot find folder $build_path/$server"
	exit -1
fi

# 拷贝文件
rm -rf $temp_pack_path
mkdir -p $temp_pack_path/$server
cp $build_path/$server/start.sh $temp_pack_path/$server
cp $build_path/$server/stop.sh $temp_pack_path/$server
cp $build_path/$server/$server $temp_pack_path/$server
cp $build_path/$server/$server.conf.example $temp_pack_path/$server/$server.conf
cp $build_path/$server/$server.mfw.conf.example $temp_pack_path/$server/$server.mfw.conf

pack_name=$app-$server-`date +%Y%m%d`.tar.gz

# pack and compress
echo ">>> begin pack and compress $pack_name ..."
tar czf $pack_name -C $temp_pack_path $server
rm -rf $temp_pack_path
echo -e ">>> end pack $pack_name ... success"
exit 0
