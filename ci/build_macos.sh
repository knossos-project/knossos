#!/bin/bash
set -euxo pipefail

PYTHON_VERSION=$(brew list --versions python | cut -d " " -f2)
PYTHON_MAJOR=$(echo ${PYTHON_VERSION} | cut -d '.' -f1)
PYTHON_MINOR=$(echo ${PYTHON_VERSION} | cut -d '.' -f2)

# Build PythonQt
time git clone --single-branch --branch new https://github.com/knossos-project/PythonQt.git || true
cd PythonQt
git fetch
git reset --hard
cd ..
mkdir -p PythonQt-build && cd PythonQt-build
rm -fv CMakeCache.txt
time cmake -G Ninja ../PythonQt -DCMAKE_PREFIX_PATH=/usr/local/opt/qt
time ninja install

QUAZIP_VERSION=$(brew list --versions quazip | cut -d " " -f2)

cd $TRAVIS_BUILD_DIR && cd ..
# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake -G Ninja ../knossos -Dpythonqt=Qt5Python${PYTHON_MAJOR}${PYTHON_MINOR} -DCMAKE_PREFIX_PATH=/usr/local/opt/qt
time ninja

# OS X housekeeping
mv knossos.app KNOSSOS.app
time /usr/local/opt/qt/bin/macdeployqt KNOSSOS.app
cp -v /usr/local/Cellar/quazip/${QUAZIP_VERSION}/lib/libquazip.1.0.0.dylib KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib
cp -v /usr/local/Frameworks/Python.framework/Versions/${PYTHON_MAJOR}.${PYTHON_MINOR}/lib/libpython${PYTHON_MAJOR}.${PYTHON_MINOR}.dylib KNOSSOS.app/Contents/Frameworks/
install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore

# Deployment
time zip -r ${TRAVIS_BUILD_DIR}/macos.KNOSSOS.nightly.app.zip KNOSSOS.app
cp -v ${TRAVIS_BUILD_DIR}/macos.KNOSSOS.nightly.app.zip ${TRAVIS_BUILD_DIR}/macos.${TRAVIS_BRANCH}-KNOSSOS.nightly.app.zip
