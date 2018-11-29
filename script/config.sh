#!/bin/bash

set -u
set -e

app=tex
rootdir=/data/tex
version_backup=/data/tex/version_backup
unpack_path=/data/tex/version_release/$app
run_path=/data/app/$app
tools_path=/data/tex/tools
mfw_script_path=$tools_path/script

build_path=../server
src_path=..

all_svr=(mfwlog mfwregistry)

function get_env_config()
{
	if [[ $# -lt 1 ]]; then
		echo "Usage: $0 (d:开发环境)"
		exit -1
	fi
	rel_env=$1
	case $rel_env in
		d)
		cc_ip=192.168.0.11
		rel_user=dev
		rel_passwd=#Bugsfor\$Dev
		registry_db_host=192.168.0.11
		registry_db_user=dev
		registry_db_pass=777777
		;;
		*)
		echo "$rel_env is not valid param, it should be \"d\" or \"r\" "
		exit 10
		;;
	esac
}
