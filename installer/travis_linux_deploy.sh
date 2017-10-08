#!/bin/bash

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

cd lib
rm -v libgnutls.so.30 libpython2.7.so.1.0 libsystemd.so.0 libpng16.so.16 libgcrypt.so.20
cd ..

rm -v *.AppImage
../deploy-tools/appimagetool . --verbose --no-appstream
