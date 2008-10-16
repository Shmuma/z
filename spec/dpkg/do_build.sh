#!/bin/sh

distr=$1
arch=$2

ssh $distr$arch<<'EOF'
cd dpkg
FNAME=`ls -1`
DNAME=`echo $FNAME | sed 's/.tar.gz//'`
DISTR=`cat ~/.zab-distr`
echo $FNAME
echo $DNAME
echo $DISTR
tar xzf $FNAME
cd $DNAME
find . -type f -exec touch '{}' ';'
debian/update-for $DISTR
dpkg-buildpackage -rfakeroot > ../log.txt 2>&1
PNAME=`ls -1 ../*.dsc | sed 's/.dsc//'`
ANAME=`dpkg-architecture | grep DEB_BUILD_ARCH= | cut -d = -f 2`
dpkg-genchanges > ${PNAME}_${ANAME}.changes
EOF
mkdir -p res/$distr/$arch
scp $distr$arch:dpkg/*$distr* res/$distr/$arch/
