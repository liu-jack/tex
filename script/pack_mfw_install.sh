#!/bin/bash

temp_mfw=./mfw

mkdir -p $temp_mfw/mfwregistry
cp /usr/local/mfw/server/mfwregistry/mfwregistry $temp_mfw/mfwregistry
cp /usr/local/mfw/server/mfwregistry/*.sh $temp_mfw/mfwregistry
cp /usr/local/mfw/server/mfwregistry/*.example $temp_mfw/mfwregistry

mkdir -p $temp_mfw/mfwlog
cp /usr/local/mfw/server/mfwlog/mfwlog $temp_mfw/mfwlog
cp /usr/local/mfw/server/mfwlog/*.sh $temp_mfw/mfwlog
cp /usr/local/mfw/server/mfwlog/*.example $temp_mfw/mfwlog

mkdir -p $temp_mfw/sql
cp ../mfwsql/db_mfw.sql $temp_mfw/sql

tar czvf mfw-1.0.tar.gz $temp_mfw
rm -rf $temp_mfw

echo ">>> success"
