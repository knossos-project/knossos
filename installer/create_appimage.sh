#!/bin/bash
set -euxo pipefail

mkdir -p deploy-tools
cd deploy-tools

curl -o linuxdeployqt -L https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
curl -o appimagetool -L https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x linuxdeployqt appimagetool
cd ..

mkdir -p deploy
cp -v knossos ../knossos/installer/knossos.desktop ../knossos/resources/icons/knossos.png deploy
cd deploy

../deploy-tools/linuxdeployqt knossos -bundle-non-qt-libs

mkdir -p glibc
tar -xf $(find /var/cache/pacman/pkg -iname $(expac "%n-%v-x86_64.pkg.tar.xz" glibc)) -C glibc

patchelf --set-rpath "\$ORIGIN/lib:\$ORIGIN/glibc/usr/lib" knossos
patchelf --set-interpreter glibc/usr/lib/ld-2.28.so knossos
patchelf --add-needed libpthread.so.0 knossos
patchelf --add-needed libQt5XcbQpa.so.5 knossos
patchelf --add-needed libdl.so.2 knossos

rm -v AppRun
cp -v ../../knossos/installer/AppRun .

#cd lib
#rm -v libgnutls.so.30 libpython2.7.so.1.0 libsystemd.so.0 libpng16.so.16 libgcrypt.so.20 # will error if one of the files isn’t present
#cd ..

rm -fv *.AppImage
env ARCH=x86_64 ../deploy-tools/appimagetool . --verbose --no-appstream
