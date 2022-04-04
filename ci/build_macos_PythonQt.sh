#!/bin/bash
set -euxo pipefail

# Build PythonQt
time git clone --single-branch --branch new https://github.com/knossos-project/PythonQt.git || true
cd PythonQt
git fetch
git reset --hard
cd ..
mkdir -p PythonQt-build && cd PythonQt-build
rm -fv CMakeCache.txt
time cmake -G Ninja ../PythonQt -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5/ -DCMAKE_CXX_FLAGS='-Wno-deprecated-declarations -Wno-register'
time ninja install
