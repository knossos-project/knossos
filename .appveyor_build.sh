#!/bin/bash

pacman -Syuu --noconfirm
pacman -S --needed --noconfirm $MINGW_PACKAGE_PREFIX-{boost,cmake,jasper,ninja,python2,qt5-static,snappy,toolchain}

# Download and install static PythonQt and QuaZIP
curl -L https://al3xst.de/stuff/$MINGW_PACKAGE_PREFIX-pythonqt-static.pkg.tar.xz > pythonqt.pkg.tar.xz
curl -L https://al3xst.de/stuff/$MINGW_PACKAGE_PREFIX-quazip-static.pkg.tar.xz > quazip.pkg.tar.xz
pacman -U --noconfirm pythonqt.pkg.tar.xz quazip.pkg.tar.xz

PROJECTPATH=$(cygpath ${APPVEYOR_BUILD_FOLDER})

# Append "-nightly" suffix in version file
sed -i -E "/KVERSION/s/(([0-9]\.?)+)/\1-nightly/g" $PROJECTPATH/version.h

mkdir knossos-build
cd knossos-build
cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RELEASE $PROJECTPATH

# Build
ninja -j2

# Deploy
if [[ $APPVEYOR_REPO_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${APPVEYOR_REPO_BRANCH}-
fi
cp knossos.exe $PROJECTPATH/win${MINGW_PREFIX: -2}.standalone.KNOSSOS.${BRANCH_PREFIX}nightly.exe
