#!/bin/bash
#agent服务器自身调用安装

if [ $# -lt 3 ]; then
	echo "Usage: $0 pack_name server_name dst_ip"
	exit -1
fi
source config.sh
pack_name=$1
server=`echo $pack_name |cut -d "-" -f2`
server_name=$2
dst_ip=$3

echo ">>> unpack $pack_name ..."
mkdir -p $unpack_path
if [ -f $version_backup/$pack_name ]; then 
	tar zxf $version_backup/$pack_name -C $unpack_path 
else
	echo "cannot find file : $version_backup/$pack_name"
	exit -1
fi

echo ">>> stop $server_name ..."
if [ -f $run_path/$server_name/stop.sh ]; then 
	echo "run stop.sh"
	$run_path/$server_name/stop.sh 
fi

echo ">>> copy $server_name ..."
mkdir -p $run_path/$server_name
cp -r $unpack_path/$server/* $run_path/$server_name
sed -i -e "s/TEMPLATE_SERVER_NAME/$server_name/g" $run_path/$server_name/start.sh
sed -i -e "s/TEMPLATE_SERVER_NAME/$server_name/g" $run_path/$server_name/stop.sh

if [ "$server" != "$server_name" ]; then
	mv $run_path/$server_name/$server $run_path/$server_name/$server_name
	mv $run_path/$server_name/$server.conf $run_path/$server_name/$server_name.conf
	mv $run_path/$server_name/$server.mfw.conf $run_path/$server_name/$server_name.mfw.conf
	server=$server_name
fi

chmod a+x $run_path/$server/*.sh
chmod a+x $run_path/$server/$server

sed -i -e "s/TEMPLATE_DBHOST/$registry_db_host/g" $run_path/$server/$server.conf
sed -i -e "s/TEMPLATE_DB_USER/$registry_db_user/g" $run_path/$server/$server.conf
sed -i -e "s/TEMPLATE_DB_PASSWD/$registry_db_pass/g" $run_path/$server/$server.conf
# endpoint config
if [ -f $run_path/$server/$server.mfw.conf ];then
	sql="select endpoint from db_tex.t_service where app='tex' and server='mfwregistry'"
        locatorpoint=$(echo "$sql"|mysql -h$registry_db_host -u$registry_db_user -p$registry_db_pass -N)
        locapoint=$(echo $locatorpoint|sed 's/[ ]\+/:/7')
        sed -i -e "s/TEMPLATE_LOCATOR/$locapoint/g" $run_path/$server/$server.mfw.conf
        for service in $(grep "TEMPLATE_ENDPOINT_" $run_path/$server/$server.mfw.conf|awk -F"TEMPLATE_ENDPOINT_" {'print $2'}); do
        	sql="select endpoint from db_tex.t_service where app='$app' and server='$server_name' and service='$service' and node='$dst_ip'
"
                endpoint=$(echo "$sql"|mysql -h$registry_db_host -u$registry_db_user -p$registry_db_pass -N)
               	sed -i -e "s/TEMPLATE_ENDPOINT_$service/$endpoint/g" $run_path/$server/$server.mfw.conf
        done
	sed -i -e "s/TEMPLATE_SERVER_NAME/$server_name/g" $run_path/$server/$server.mfw.conf
fi

echo ">>> start $server ..."
$run_path/$server/start.sh 
