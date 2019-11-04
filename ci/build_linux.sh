#!/bin/bash
set -euxo pipefail

time pacman -Sy archlinux-keyring --noconfirm
time pacman -Syu --noconfirm

cd ~

# run cmake and ninja
mkdir knossos-build
cd knossos-build
time cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_PREFIX_PATH="/root/PythonQt-install/lib/cmake/" ../knossos
time ninja

# create AppImage
time ../knossos/installer/create_appimage.sh

# Deploy
cp deploy/*.AppImage ../knossos/linux.KNOSSOS.nightly.AppImage
cp deploy/*.AppImage ../knossos/linux.${TRAVIS_BRANCH}-KNOSSOS.nightly.AppImage
