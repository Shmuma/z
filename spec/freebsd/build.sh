#!/bin/sh

cd ../../
./configure --enable-agent --prefix=`pwd`/spec/freebsd && make && make install
