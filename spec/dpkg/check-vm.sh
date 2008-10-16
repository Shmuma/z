#!/bin/sh

for distr in etch sarge hardy feisty gutsy; do
    for arch in 32 64; do
	echo -n $distr$arch: 
	ssh $distr$arch hostname -f
    done
done
