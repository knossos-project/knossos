#!/bin/bash
set -euxo pipefail

# don’t, because slow and unnecessary
sed -i 's/CheckSpace/#CheckSpace/' /etc/pacman.conf
# no need wasting time updating msys and 32 bits
sed -i "s?#IgnorePkg   =?IgnorePkg   = $(pacman -Slq msys mingw32 | xargs pacman -Qq 2>/dev/null | tr '\n' ' ') ?" /etc/pacman.conf

# don’t verify twice, only during install
sed -i 's/Required/Never/' /etc/pacman.conf
time pacman --noconfirm --needed -Syuuw ${MINGW_PACKAGE_PREFIX}-{boost,cmake,jasper,ninja,python,qt5-static,snappy,toml11,toolchain}
# Download and install static PythonQt and QuaZIP, also only directly works like this with disabled signature checking
time pacman --noconfirm -U https://github.com/knossos-project/knossos/releases/download/nightly-dev/mingw-w64-MINGW64-{pythonqt,quazip}-static.pkg.tar.zst
sed -i 's/Never/Required/' /etc/pacman.conf
time pacman --noconfirm --needed -Syuu ${MINGW_PACKAGE_PREFIX}-{boost,cmake,jasper,ninja,python,qt5-static,snappy,toolchain}

PROJECTPATH=$(cygpath ${APPVEYOR_BUILD_FOLDER})

mkdir knossos-build
cd knossos-build
DEBUG_FLAGS=-DCMAKE_CXX_FLAGS="-g -fno-omit-frame-pointer"
time cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RELEASE "${DEBUG_FLAGS}" ${PROJECTPATH}

# Build
time ninja

# Deploy
cp -v knossos.exe $PROJECTPATH/win.${APPVEYOR_REPO_BRANCH}-KNOSSOS.nightly.exe
strip -v knossos.exe
mv -v knossos.exe $PROJECTPATH/win.KNOSSOS.nightly.exe
