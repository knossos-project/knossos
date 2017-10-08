#!/bin/sh

#run cmake and ninja
cd ..
cd knossos-release
cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE -DDEPLOY=TRUE -DCMAKE_PREFIX_PATH="/root/PythonQt-install/lib/cmake/" -DDEPLOY=TRUE ../knossos; ninja -j3

if [ $? -eq 0 ]; then
        echo "built succeeded"
else
        echo "built failed"
        exit $?
fi

#create .deb
cd ../knossos/installer
sed -i 's/read/#read/' create-deb.sh #so we dont have to press enter
./create-deb.sh

#create appimage
mkdir KNOSSOS
cp knossos.deb KNOSSOS
curl -L https://github.com/probonopd/AppImages/raw/master/recipes/meta/Recipe > recipe
chmod +x recipe
./recipe KNOSSOS.yml

#rename appimage and move to artifact folder for deployment
cd out
mv KNOSSOS-.glibc*-x86_64.AppImage linux64.KNOSSOS.nightly.AppImage

#copy file to artifact folder
cp linux64.KNOSSOS.nightly.AppImage /root/artifact

