#!/bin/bash

[ ! -d build ] && mkdir build;

old_path=`pwd`

cd build

cmake ..
make -j8 && make install

cd ..
cp -rf build/tex-1.0/* tex
