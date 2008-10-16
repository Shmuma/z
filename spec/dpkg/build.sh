#!/bin/sh

rm -rf res

for distr in etch sarge hardy feisty gutsy; do
    for arch in 32 64; do
        ssh $distr$arch "rm -rf dpkg; mkdir dpkg; echo $distr > ~/.zab-distr"
        scp $1 $distr$arch:dpkg/
        ./do_build.sh $distr $arch &
    done
done
