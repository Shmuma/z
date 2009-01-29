#!/bin/sh

cd ../../
./configure --enable-agent --prefix=`pwd`/spec/freebsd/usr/local --sysconfdir=/usr/local/etc/ && make && make install
