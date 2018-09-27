#!/bin/bash
set -euxo pipefail

pacman -Syu --noconfirm

cd ~

# run cmake and ninja
mkdir knossos-build
cd knossos-build
cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE -DDEPLOY=TRUE -DCMAKE_PREFIX_PATH="/root/PythonQt-install/lib/cmake/" ../knossos
ninja

# create AppImage
../knossos/installer/create_appimage.sh

# Deploy
BRANCH_PREFIX=""
if [[ $TRAVIS_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${TRAVIS_BRANCH}-
fi
cp deploy/*.AppImage ../knossos/linux.${BRANCH_PREFIX}KNOSSOS.nightly.AppImage
