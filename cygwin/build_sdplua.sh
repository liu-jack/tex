#!/bin/bash
cd $(dirname $0)

CXX=x86_64-w64-mingw32-g++
CXX_FLAGS="-static -shared"
OUTPUT=bin/sdplua.dll
mkdir -p bin

LUA_LINK="-L /cygdrive/e/share/lua-5.1.5-win64/ -l lua5.1"
LUA_INCLUDE="-I /cygdrive/e/share/lua-5.1.5_Win64_dll8_lib/include/"

SRC=../src/sdp/sdp2lua/lua/sdplua.cpp
INCLUDE="-I ../src/sdp"

$CXX $CXX_FLAGS $INCLUDE $LUA_INCLUDE -o $OUTPUT $SRC $LUA_LINK
