#!/bin/bash

pacman -Syu --noconfirm
pacman --sync --noconfirm mingw-w64-x86_64-toolchain # msys64-only
pacman -S mingw-w64-x86_64-boost --noconfirm
pacman -S mingw-w64-x86_64-python2 --noconfirm
pacman -S mingw-w64-x86_64-qt5-static --noconfirm
pacman -S mingw-w64-x86_64-ninja --noconfirm
pacman -S mingw-w64-x86_64-snappy --noconfirm
pacman -S mingw-w64-x86_64-jasper --noconfirm #jpeg-2000
pacman -S mingw-w64-x86_64-cmake --noconfirm #cmake

####
# Download and unpack PythonQt
####
curl -L https://al3xst.de/stuff/pythonqt.tar.xz > pythonqt.tar.xz
tar xvf pythonqt.tar.xz

####
### apply *** TEMP workaround *** for PythonQt
####
PROJECTPATH=`ls /c/projects/`
echo 'get_target_property(gui_static_plugins Qt5::Gui STATIC_PLUGINS)' >> /c/projects/${PROJECTPATH}/CMakeLists.txt
echo 'if(gui_static_plugins)' >> /c/projects/${PROJECTPATH}/CMakeLists.txt
echo 'string(REGEX REPLACE [;]*QVirtualKeyboardPlugin[;]* ";" gui_static_plugins_new "${gui_static_plugins}")' >> /c/projects/${PROJECTPATH}/CMakeLists.txt
echo 'set_target_properties(Qt5::Gui PROPERTIES STATIC_PLUGINS "${gui_static_plugins_new}")' >> /c/projects/${PROJECTPATH}/CMakeLists.txt
echo 'endif()' >> /c/projects/${PROJECTPATH}/CMakeLists.txt

###
# append "-nightly" suffix in version file
###
KVER=`cat /c/projects/${PROJECTPATH}/version.h | grep KVERSION | cut -d'"' -f2` #get version name aka 5.0
sed -i "s/KVERSION \""${KVER}"\"/KVERSION \"${KVER}-nightly\"/g" /c/projects/${PROJECTPATH}/version.h #append -nightly, so version is 5.0-nightly

####
# Download and install Quazip-Static
####
curl -L https://al3xst.de/stuff/mingw-w64-x86_64-quazip-static-0.7-1-any.pkg.tar.xz > quazip-static.tar.xz
pacman -U quazip-static.tar.xz --noconfirm

####
# Run cmake
####
mkdir knossos-build
cd knossos-build
cmake -G Ninja -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_PREFIX_PATH="/c/projects/${PROJECTPATH}/mingw64/qt5-static/" /c/projects/${PROJECTPATH}

####
# build knossos
####
ninja -j2

if [ $APPVEYOR_REPO_BRANCH = "master" ]; then
	cp knossos.exe /c/projects/${PROJECTPATH}/win64.standalone.KNOSSOS.nightly.exe
else
	cp knossos.exe /c/projects/${PROJECTPATH}/win64.standalone.KNOSSOS.${APPVEYOR_REPO_BRANCH}-nightly.exe
fi
