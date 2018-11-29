#!/bin/bash
# 打包tools并上传到CC
if [ $# -ne 1 ] ;then
	echo "Usage: $0  env(\"d\" or \"r\" or \"u\")"
	exit -1
fi

source config.sh
get_env_config $1
source remote_cmd.sh

cc_tools=tools
mkdir -p $cc_tools

echo ">>> begin pack tools file ..."

# script
scriptdir=`pwd`
mkdir -p $cc_tools/script
cp -rf $scriptdir/config.sh $cc_tools/script
echo -e "\nget_env_config $1" >> $cc_tools/script/config.sh
cp -rf $scriptdir/remote_cmd.sh $cc_tools/script
cp -rf $scriptdir/remote_cmd.exp $cc_tools/script
cp -rf $scriptdir/func.sh $cc_tools/script
cp -rf $scriptdir/agent_install_server.sh $cc_tools/script
cp -rf $scriptdir/cc_dispatch_server_ip.sh $cc_tools/script
cp -rf $scriptdir/cc_dispatch_tools_ip.sh $cc_tools/script
cp -rf $scriptdir/cc_release_server.sh $cc_tools/script

pack_tools_name=$app-tools-`date +%Y%m%d`.tar.gz
tar czf $pack_tools_name $cc_tools/script

runcmd $rel_user@$cc_ip "mkdir -p $mfw_script_path; mkdir -p $rootdir/tmp"
putfile $rel_user@$cc_ip $pack_tools_name $rootdir/tmp
runcmd $rel_user@$cc_ip "tar zxf $rootdir/tmp/$pack_tools_name -C $rootdir"

rm -rf $cc_tools
rm $pack_tools_name
echo -e ">>> $0 success\n"

exit 0
