#!/bin/bash
appname=knossos
mkdir -p $appname/opt/$appname/platforms
cp /usr/lib/qt/plugins/platforms/libqxcb.so $appname/opt/$appname/platforms
cp -r /usr/lib/qt/plugins/imageformats $appname/opt/$appname/
cp -r /usr/lib/qt/plugins/xcbglintegrations $appname/opt/$appname/
cp -v ../../knossos-release/$appname $appname/opt/$appname/$appname
./copy_linked_libraries.bash $appname/opt/$appname/platforms/libqxcb.so $appname/opt/$appname/
./copy_linked_libraries.bash $appname/opt/$appname/$appname $appname/opt/$appname/
sed -i -r 's/Installed-Size:.*/Installed-Size: '$(./package-size.sh $appname | egrep -o [0-9]+)'/g' $appname/DEBIAN/control
fakeroot dpkg -b $appname
read -p "done – press enter to continue"
