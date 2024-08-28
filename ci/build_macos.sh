#!/bin/bash
set -euxo pipefail

QUAZIP_VERSION=$(brew list --versions quazip | tr ' ' '\n' | tail -1)

cd ${APPVEYOR_BUILD_FOLDER} && cd ..
# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake -G Ninja ../knossos -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5/ -DCMAKE_CXX_FLAGS=-Wno-deprecated-declarations -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
time ninja

# OS X housekeeping
mv knossos.app KNOSSOS.app
time /usr/local/opt/qt@5/bin/macdeployqt KNOSSOS.app
cp -v /usr/local/lib/libquazip1-qt5.${QUAZIP_VERSION}.dylib KNOSSOS.app/Contents/Frameworks/libquazip*.dylib
install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip*.dylib -change /usr/local/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore

PYTHON_VERSION=$(cd /usr/local/Frameworks/Python.framework/Versions/ && echo * | tr ' ' '\n' | grep -v Current | sort -V | tail -1)
mkdir -p KNOSSOS.app/Contents/Frameworks/Python.framework/Versions
cp -r /usr/local/Frameworks/Python.framework/Versions/${PYTHON_VERSION} KNOSSOS.app/Contents/Frameworks/Python.framework/Versions
install_name_tool KNOSSOS.app/Contents/MacOS/knossos -change /usr/local/opt/python@${PYTHON_VERSION}/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python @executable_path/../Frameworks/Python.framework/Versions/${PYTHON_VERSION}/Python

cp -v /usr/local/opt/openssl/lib/lib{ssl,crypto}*.dylib KNOSSOS.app/Contents/Frameworks/
install_name_tool KNOSSOS.app/Contents/Frameworks/Python.framework/Versions/*/lib/python*/lib-dynload/_ssl.cpython-*-darwin.so -change /usr/local/opt/openssl@3/lib/libssl.3.dylib @executable_path/../Frameworks/libssl.3.dylib
install_name_tool KNOSSOS.app/Contents/Frameworks/Python.framework/Versions/*/lib/python*/lib-dynload/_ssl.cpython-*-darwin.so -change /usr/local/opt/openssl@3/lib/libcrypto.*.dylib @executable_path/../Frameworks/libcrypto.3.dylib
install_name_tool KNOSSOS.app/Contents/Frameworks/libssl.3.dylib -change /usr/local/Cellar/openssl@3/*/lib/libcrypto.3.dylib @executable_path/../Frameworks/libcrypto.3.dylib

# Deployment
time zip -rq ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip KNOSSOS.app
cp -v ${APPVEYOR_BUILD_FOLDER}/macos.KNOSSOS.nightly.app.zip ${APPVEYOR_BUILD_FOLDER}/macos.${APPVEYOR_REPO_BRANCH}-KNOSSOS.nightly.app.zip
