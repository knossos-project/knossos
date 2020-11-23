#!/bin/bash
set -euxo pipefail

# Build PythonQt
time git clone --single-branch --branch new https://github.com/knossos-project/PythonQt.git
mkdir PythonQt-build && cd PythonQt-build
time cmake -G Ninja ../PythonQt -DCMAKE_PREFIX_PATH=/usr/local/opt/qt
time ninja install

# Download and install PythonQt
#time curl -JLO https://github.com/knossos-project/knossos/releases/download/nightly-dev/macOS-PythonQt.zip
#time unzip -d / macOS-PythonQt.zip

# Fix QuaZip include directory name
cd $TRAVIS_BUILD_DIR && cd ..
mkdir -p quazip/quazip5
QUAZIP_VERSION=`brew list --versions quazip | cut -d " " -f2`
time cp -R /usr/local/Cellar/quazip/${QUAZIP_VERSION}/include/quazip/* quazip/quazip5/

# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake -G Ninja ../knossos -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DCMAKE_CXX_FLAGS=-isystem\ ${TRAVIS_BUILD_DIR}/../quazip
time ninja

# OS X housekeeping
mv knossos.app KNOSSOS.app
time /usr/local/opt/qt/bin/macdeployqt KNOSSOS.app
cp -v /usr/local/Cellar/quazip/${QUAZIP_VERSION}/lib/libquazip.1.0.0.dylib KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib
cp -v /usr/local/Frameworks/Python.framework/Versions/3.9/lib/libpython3.9.dylib KNOSSOS.app/Contents/Frameworks/
install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore

# Deployment
time zip -r ${TRAVIS_BUILD_DIR}/macos.KNOSSOS.nightly.app.zip KNOSSOS.app
cp -v ${TRAVIS_BUILD_DIR}/macos.KNOSSOS.nightly.app.zip ${TRAVIS_BUILD_DIR}/macos.${TRAVIS_BRANCH}-KNOSSOS.nightly.app.zip
