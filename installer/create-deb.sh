#!/bin/bash
appname=knossos-nightly
mkdir -p $appname/opt/$appname/platforms
mkdir -p $appname/opt/$appname/sqldrivers
cp -v /home/knossos/Dokumente/build-knossos-release-Desktop-Default/knossos $appname/opt/$appname/
fakeroot dpkg -b $appname
read -p "done – press enter to continue"
