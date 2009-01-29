#!/bin/sh

cd ../../
./configure --enable-agent --prefix=`pwd`/spec/freebsd/usr/local && make && make install
