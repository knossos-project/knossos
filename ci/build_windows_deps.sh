#!/bin/bash
set -euxo pipefail

# don’t, because slow and unnecessary
sed -i 's/CheckSpace/#CheckSpace/' /etc/pacman.conf
# no need wasting time updating msys and 32 bits
sed -i "s?#IgnorePkg   =?IgnorePkg   = $(pacman -Slq msys mingw32 | xargs pacman -Qq 2>/dev/null | tr '\n' ' ') ?" /etc/pacman.conf

# don’t verify twice, only during install
sed -i 's/Required/Never/' /etc/pacman.conf
deps=$(echo ${MINGW_PACKAGE_PREFIX}-{boost,cmake,jasper,ninja,qt5-static,snappy,toolchain})
time pacman --noconfirm --needed -Syuuw ${deps}
sed -i 's/Never/Required/' /etc/pacman.conf
time pacman --noconfirm --needed -Syuu ${deps} mingw-w64-ucrt-x86_64-binutils

PROJECTPATH=$(cygpath ${APPVEYOR_BUILD_FOLDER})

cd $PROJECTPATH/ci/deps
for dep in $(ls); do
	cd $PROJECTPATH/ci/deps/$dep
	makepkg-mingw -s --noconfirm
	# Deploy
	mv -v *.pkg.tar.zst $PROJECTPATH/mingw-w64-${MSYSTEM}-${dep}.pkg.tar.zst
done
