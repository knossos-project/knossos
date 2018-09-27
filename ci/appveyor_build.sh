#!/bin/bash
set -euxo pipefail

pacman -Syuu --needed --noconfirm $MINGW_PACKAGE_PREFIX-{boost,cmake,jasper,ninja,python2,qt5-static,snappy,toolchain}

# Download and install static PythonQt and QuaZIP
curl -OJ https://al3xst.de/stuff/$MINGW_PACKAGE_PREFIX-pythonqt-static.pkg.tar.xz
curl -OJ https://al3xst.de/stuff/$MINGW_PACKAGE_PREFIX-quazip-static.pkg.tar.xz
pacman -U --noconfirm $MINGW_PACKAGE_PREFIX-{pythonqt,quazip}-static.pkg.tar.xz

PROJECTPATH=$(cygpath ${APPVEYOR_BUILD_FOLDER})

mkdir knossos-build
cd knossos-build
cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RELEASE $PROJECTPATH

# Build
ninja

# Deploy
BRANCH_PREFIX=""
if [[ $APPVEYOR_REPO_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${APPVEYOR_REPO_BRANCH}-
fi
cp knossos.exe $PROJECTPATH/win.${BRANCH_PREFIX}KNOSSOS.nightly.exe
