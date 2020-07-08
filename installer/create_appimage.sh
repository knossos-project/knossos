#!/bin/bash
set -euxo pipefail

mkdir -p deploy-tools
cd deploy-tools

curl --fail -C - -o linuxdeployqt -L https://github.com/probonopd/linuxdeployqt/releases/download/5/linuxdeployqt-5-x86_64.AppImage
curl --fail -C - -o appimagetool -L https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x linuxdeployqt appimagetool
cd ..

mkdir -p deploy
cp -v knossos ../knossos/installer/knossos.desktop ../knossos/resources/icons/knossos.png deploy
cd deploy

../deploy-tools/linuxdeployqt knossos -bundle-non-qt-libs

mkdir -p glibc
tar -xf $(find /var/cache/pacman/pkg -iname $(expac "%n-%v-x86_64.pkg.tar.zst" glibc)) -C glibc

patchelf --set-interpreter glibc/usr/lib/ld-*.so knossos
patchelf --add-needed libpthread.so.0 knossos
patchelf --add-needed libQt5XcbQpa.so.5 knossos
patchelf --add-needed libdl.so.2 knossos

cp -av /usr/lib/libstdc++.so.* lib/
cp -av /usr/lib/libz.so.* lib/
cp -av /usr/lib/libgpg-error.so.* lib/
cp -av /usr/lib/libxcb.so.* lib/
cp -av /usr/lib/libharfbuzz.so.* lib/
cp -av /usr/lib/libfreetype.so.* lib/

rm -v AppRun
cp -v ../../knossos/installer/AppRun .

cd lib
rm -v libpython2.7.so.1.0
mkdir -p supplemental
mv -v libGLdispatch.so.0 supplemental/
cd ..

rm -fv *.AppImage
env ARCH=x86_64 ../deploy-tools/appimagetool . --verbose --no-appstream
