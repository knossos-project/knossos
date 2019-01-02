#!/bin/bash
set -euxo pipefail

# Fix Python headers
# https://bugs.python.org/review/10910/diff2/2561:8559/Include/pyport.h
cd ..
mkdir python2.7
time cp /usr/include/python2.7/* python2.7
ed -s python2.7/pyport.h << EOF
/#ifdef _PY_PORT_CTYPE_UTF8_ISSUE/
a
#ifndef __cplusplus
.
+16
a
#endif
.
w
EOF

# Build PythonQt
time git clone https://github.com/knossos-project/PythonQt.git
mkdir PythonQt-build && cd PythonQt-build
time cmake ../PythonQt -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DPYTHON_INCLUDE_DIR=${TRAVIS_BUILD_DIR}/../python2.7
time make install

# Fix QuaZip include directory name
cd $TRAVIS_BUILD_DIR && cd ..
mkdir -p quazip/quazip5
QUAZIP_VERSION=`brew list --versions quazip | cut -d " " -f2`
time cp -R /usr/local/Cellar/quazip/${QUAZIP_VERSION}/include/quazip/* quazip/quazip5/

# Build KNOSSOS
mkdir knossos-build && cd knossos-build
time cmake ../knossos -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DCMAKE_CXX_FLAGS=-isystem\ ${TRAVIS_BUILD_DIR}/../quazip
time make

# OS X housekeeping
mv knossos.app KNOSSOS.app
time /usr/local/opt/qt/bin/macdeployqt KNOSSOS.app
cp /usr/local/Cellar/quazip/${QUAZIP_VERSION}/lib/libquazip.1.0.0.dylib KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib
install_name_tool KNOSSOS.app/Contents/Frameworks/libquazip.1.dylib -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore

# Deployment
BRANCH_PREFIX=""
if [[ $TRAVIS_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${TRAVIS_BRANCH}-
fi
time zip -r ${TRAVIS_BUILD_DIR}/macos.${BRANCH_PREFIX}KNOSSOS.nightly.app.zip KNOSSOS.app
