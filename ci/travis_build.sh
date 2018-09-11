#!/bin/sh

pacman -Syu --noconfirm

# run cmake and ninja
cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE -DDEPLOY=TRUE -DCMAKE_PREFIX_PATH="/root/PythonQt-install/lib/cmake/" ../knossos
ninja -j3

if [ $? -eq 0 ]; then
        echo "built succeeded"
else
        echo "built failed"
        exit $?
fi
# create AppImage
../knossos/installer/create_appimage.sh
# copy file to artifact folder
# Deploy
if [[ $TRAVIS_BRANCH != "master" ]]; then
	BRANCH_PREFIX=${TRAVIS_BRANCH}-
fi
cp deploy/*.AppImage /root/artifact/linux.${BRANCH_PREFIX}KNOSSOS.nightly.AppImage
