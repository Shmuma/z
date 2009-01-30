#!/bin/sh

# return size of route table
netstat -rn | grep -c " U"