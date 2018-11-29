#!/bin/bash

source config.sh

function getPackName()
{
	if [ $# -ne 3 ]; then echo "$0 ret_pack_name svr_name version"; exit 1; fi
	local __retVal=$1

	if ! svn info $src_path >/dev/null ; then
		exit 2
	fi
	local svn_version=`svn info $src_path|grep Revision|sed 's/Revision: //g'`
	local ret_name="$app-$2-$3-`date +%Y%m%d`-$svn_version.tar.gz"

	eval $__retVal="'$ret_name'"
	return 0
}

# 检查输入的Svr名字是不是可识别的名字
function checkSvrName()
{
	if [ $# -lt 1 ]; then echo "$0 svr_name"; exit 1; fi

	local svrPrefix="$1"
	svrPrefix=`echo $svrPrefix | cut -d '_' -f 1`

	for svr in ${all_svr[@]}; do
		if [ "$svr" = "$svrPrefix" ]; then
			return 0
		fi
	done

	return 1
}

# 获取ServerIP
function getServerIp()
{
	#check param
	if [ $# -lt 3 ]; then echo "$0 -r retIPs -s server -n server_name"; exit 1; fi
	OPTIND=1
	while getopts ":s:d:r:" arg
	do
		case $arg in
			s) local server=$OPTARG; ;;
			r) local __retVal=$OPTARG; ;;
			n) local server_name=$OPTARG; ;;
		esac
	done
	if [ -z $__retVal ]  ;then echo "$0 retValue must be set!"; exit 1 ;fi
	set +u
	if [ ! -z $server ] ;then
		if ! checkSvrName $server ;then echo "$server is not valid server name!" ; exit 1 ;fi
	fi

	# select from db
	local sql="select DISTINCT node from db_tex.t_server where app='$app'"
	if [ ! -z $server ]; then
		sql=${sql}" and server='$server'"
	fi
	set -u
	local __nodes=$(echo $sql|mysql -h$registry_db_host -u$registry_db_user -p$registry_db_pass -N)
	for ip in __nodes; do
		if [ -z $ip ]; then
			echo "error, can not find __nodes, sql: $sql"
			exit 2
		fi
	done

	eval $__retVal="'$__nodes'"
	return 0
}
