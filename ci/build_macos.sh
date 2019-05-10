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
time cmake -G Ninja ../PythonQt -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DPYTHON_INCLUDE_DIR=${TRAVIS_BUILD_DIR}/../python2.7
time ninja install

time zip -r ${TRAVIS_BUILD_DIR}/macOS-PythonQt.zip /usr/local/lib/libQt5Python27.a /usr/local/lib/libQt5Python27_QtAll.a /usr/local/include/Qt5Python27 /usr/local/lib/cmake/Qt5Python27 /usr/local/lib/cmake/Qt5Python27_QtAll
