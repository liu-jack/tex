#!/bin/bash
cd $(dirname $0)

CXX=x86_64-w64-mingw32-g++
STRIP=x86_64-w64-mingw32-strip
CXX_FLAGS="-static -DBUILD_ON_MINGW"
OUTPUT=bin/sdp2php
mkdir -p bin

SRC="../src/sdp/sdp2php/sdp2php.cpp ../src/parse/*.cpp ../src/parse/generated/*.cpp"
SRC="$SRC ../src/util/util_file.cpp ../src/util/util_string.cpp ../src/util/util_option.cpp ../src/util/util_encode.cpp"
INCLUDE="-I ../src -I ../src/parse -I ../src/parse/generated"

WIN32_LINK="-l shlwapi"

$CXX $CXX_FLAGS $INCLUDE $LUA_INCLUDE -o $OUTPUT $SRC $WIN32_LINK
$STRIP -s $OUTPUT
