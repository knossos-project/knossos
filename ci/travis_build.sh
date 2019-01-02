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
BRANCH_PREFIX=""
if [[ $TRAVIS_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${TRAVIS_BRANCH}-
fi
cp deploy/*.AppImage ../knossos/linux.${BRANCH_PREFIX}KNOSSOS.nightly.AppImage
