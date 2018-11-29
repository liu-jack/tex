#!/bin/bash

if [[ $# -lt 2 ]]; then
	echo "Usage: $0 pack_name server_name"
	exit -1
fi

source config.sh
source remote_cmd.sh
source func.sh
pack_name="$1"
server="$2"
if ! checkSvrName $server ; then
	echo "$server is not server name"
	exit 2
fi

getServerIp -s $server -r global_ip
for ip in $global_ip; do
	./cc_dispatch_server_ip.sh $pack_name $ip
	runcmd $rel_user@$ip "cd $mfw_script_path; ./agent_install_server.sh $pack_name $server $ip"
done

echo ">>> $0 success"
exit 0
