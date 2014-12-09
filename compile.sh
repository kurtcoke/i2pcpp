#!/bin/sh

# Check if dir exists else create it. (Should stil code this.
mkdir build
cd build
cmake -DBOOST_ROOT=/usr/include/boost -DBOTAN_INCLUDE_DIR=/usr/local/include/botan-1.11/ -DBOTAN_LIBRARYDIR=/usr/local/lib/ -DCMAKE_BUILD_TYPE=Debug ../
make

