#!/bin/bash
if [[ $# -ne 1 ]]; then
        echo "Usage: $0 dst_ip"
        exit -1
fi

source config.sh
source remote_cmd.sh
dst_ip=$1

agent_tools=tmp_tools
agent_tools_pack=agent-tools.tar.gz

mkdir -p $agent_tools
cp config.sh $agent_tools
cp func.sh $agent_tools
cp agent_install_server.sh $agent_tools

tar czf $agent_tools_pack -C $agent_tools ./

runcmd $rel_user@$dst_ip "mkdir -p $mfw_script_path;mkdir -p $rootdir/tmp"
putfile $rel_user@$dst_ip $agent_tools_pack $rootdir/tmp
runcmd $rel_user@$dst_ip "tar zxf $rootdir/tmp/$agent_tools_pack -C $mfw_script_path "

rm $agent_tools_pack
rm -rf $agent_tools
