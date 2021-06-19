#!/bin/bash
set -euxo pipefail

PYTHON_VERSION=$(brew list --versions python | tr " " "\n" | tail -1)
PYTHON_MAJOR=$(echo ${PYTHON_VERSION} | cut -d '.' -f1)
PYTHON_MINOR=$(echo ${PYTHON_VERSION} | cut -d '.' -f2)

QUAZIP_VERSION=$(brew list --versions quazip | tr " " "\n" | tail -1)

cd ${APPVEYOR_BUILD_FOLDER} && cd ..
# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake -G Ninja ../knossos -Dpythonqt=Qt5Python${PYTHON_MAJOR}${PYTHON_MINOR} -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5/
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

cp -r /usr/local/Frameworks/Python.framework KNOSSOS.app/Contents/Frameworks/
rm -rf KNOSSOS.app/Contents/Frameworks/Python.framework/Versions/2.7
install_name_tool KNOSSOS.app/Contents/MacOS/knossos -change /usr/local/opt/python@${PYTHON_MAJOR}.${PYTHON_MINOR}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR}.${PYTHON_MINOR}/Python @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_MAJOR}.${PYTHON_MINOR}/Python

# Deployment
time zip -r ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip KNOSSOS.app
cp -v ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip ${APPVEYOR_BUILD_FOLDER}/macos.${APPVEYOR_REPO_BRANCH}-KNOSSOS.nightly.app.zip
