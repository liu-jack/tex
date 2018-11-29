#!/bin/bash

[ ! -d build ] && mkdir build;

old_path=`pwd`

cd build

cmake ..
make -j8 && make install
