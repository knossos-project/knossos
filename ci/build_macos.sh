#!/bin/bash
set -euxo pipefail

QUAZIP_VERSION=$(brew list --versions quazip | tr ' ' '\n' | tail -1)

cd ${APPVEYOR_BUILD_FOLDER} && cd ..
# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake -G Ninja ../knossos -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5/ -DCMAKE_CXX_FLAGS=-Wno-deprecated-declarations
time ninja

# OS X housekeeping
mv knossos.app KNOSSOS.app
time /usr/local/opt/qt@5/bin/macdeployqt KNOSSOS.app
cp -v /usr/local/Cellar/quazip/${QUAZIP_VERSION}/lib/libquazip.1.0.0.dylib KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib &&
  install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib -change /usr/local/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ||
  true
cp -v /usr/local/lib/libquazip1-qt5.${QUAZIP_VERSION}.dylib KNOSSOS.app/Contents/Frameworks/libquazip1-qt5.1.dylib &&
  install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip1-qt5.1.dylib -change /usr/local/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ||
  true

PY_VER=$(cd /usr/local/Frameworks/Python.framework/Versions/ && echo * | tr ' ' '\n' | grep -v Current | sort -V | tail -1)
mkdir -p KNOSSOS.app/Contents/Frameworks/Python.framework/Versions
cp -r /usr/local/Frameworks/Python.framework/Versions/${PY_VER} KNOSSOS.app/Contents/Frameworks/Python.framework/Versions
install_name_tool KNOSSOS.app/Contents/MacOS/knossos -change /usr/local/opt/python@${PY_VER}/Frameworks/Python.framework/Versions/${PY_VER}/Python @executable_path/../Frameworks/Python.framework/Versions/${PY_VER}/Python

# Deployment
time zip -rq ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip KNOSSOS.app
cp -v ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip ${APPVEYOR_BUILD_FOLDER}/macos.${APPVEYOR_REPO_BRANCH}-KNOSSOS.nightly.app.zip
