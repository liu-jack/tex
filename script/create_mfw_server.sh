#!/bin/bash

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 App Server Service"
  exit -1
fi

SCRIPTDIR=$(dirname $0)
APP=$1
SERVER=$2
SERVICE=$3
SERVER_UPPER=$(echo $SERVER | tr a-z A-Z)
SERVICE_UPPER=$(echo $SERVICE | tr a-z A-Z)

if [[ "$SERVER" == "$SERVICE" ]]; then
  echo "Server equals to Service"
  exit -1
fi

mkdir -p ./$APP/$SERVER
cp -a $SCRIPTDIR/CMakeLists.txt ./$APP
cp -a $SCRIPTDIR/DemoServer/* ./$APP/$SERVER

(find ./$APP/$SERVER -type f; ls -1 ./$APP/CMakeLists.txt) | xargs -I{} sed -i -e "s/DemoApp/$APP/g; s/DemoServer/$SERVER/g; s/DEMOSERVER/$SERVER_UPPER/g; s/DemoService/$SERVICE/g; s/DEMOSERVICE/$SERVICE_UPPER/g" {}
rename "DemoServer" "$SERVER" ./$APP/$SERVER/* ./$APP/$SERVER/*/*
rename "DemoService" "$SERVICE" ./$APP/$SERVER/* ./$APP/$SERVER/*/*
