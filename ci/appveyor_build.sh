#!/bin/bash
set -euxo pipefail

time pacman -Syuu --needed --noconfirm $MINGW_PACKAGE_PREFIX-{boost,cmake,jasper,ninja,python2,qt5-static,snappy,toolchain}

# Download and install static PythonQt and QuaZIP
time curl -JLO https://github.com/knossos-project/knossos/releases/download/nightly-dev/$MINGW_PACKAGE_PREFIX-pythonqt-static.pkg.tar.xz
time curl -JLO https://github.com/knossos-project/knossos/releases/download/nightly-dev/$MINGW_PACKAGE_PREFIX-quazip-static.pkg.tar.xz
time pacman -U --noconfirm $MINGW_PACKAGE_PREFIX-{pythonqt,quazip}-static.pkg.tar.xz

PROJECTPATH=$(cygpath ${APPVEYOR_BUILD_FOLDER})

mkdir knossos-build
cd knossos-build
time cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RELEASE $PROJECTPATH

# Build
time ninja

# Deploy
BRANCH_PREFIX=""
if [[ $APPVEYOR_REPO_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${APPVEYOR_REPO_BRANCH}-
fi
cp knossos.exe $PROJECTPATH/win.${BRANCH_PREFIX}KNOSSOS.nightly.exe
