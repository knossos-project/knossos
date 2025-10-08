#!/bin/bash
set -euxo pipefail

time pacman -Sy archlinux-keyring --noconfirm
time pacman -Syu --noconfirm
time pacman -S --noconfirm qtkeychain-qt5 # TODO: move it to the Docker image

cd ~
git config --global --add safe.directory /root/knossos

# run cmake and ninja
mkdir knossos-build
cd knossos-build
time cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE ../knossos
time ninja

# create AppImage
time ../knossos/installer/create_appimage.sh

# Deploy
cp -v *.AppImage ../knossos/linux.KNOSSOS.nightly.AppImage
cp -v *.AppImage ../knossos/linux.${APPVEYOR_REPO_BRANCH}-KNOSSOS.nightly.AppImage
