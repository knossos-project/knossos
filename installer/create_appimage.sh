#!/bin/bash
set -euo pipefail

mkdir -p deploy-tools
cd deploy-tools
curl -C - -Lo linuxdeploy https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
curl -C - -LO https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
curl -C - -LO https://github.com/linuxdeploy/linuxdeploy-plugin-checkrt/releases/download/continuous/linuxdeploy-plugin-checkrt-x86_64.sh
curl -C - -Lo appimagetool https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x linuxdeploy linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-plugin-checkrt-x86_64.sh appimagetool
cd ..

rm -vf depl/usr/bin/knossos
time env NO_STRIP=true deploy-tools/linuxdeploy --appdir depl -e knossos -d ../knossos/installer/knossos.desktop -i ../knossos/resources/icons/knossos.png --plugin qt --plugin checkrt

#time deploy-tools/linuxdeploy --appdir depl --output appimage

cd depl

cp -av /usr/lib/libz.so.* usr/lib/
cp -av /usr/lib/libharfbuzz.so* usr/lib/
cp -av /usr/lib/libfreetype.so* usr/lib/

cp -av /usr/lib/libfontconfig.so* usr/lib/
#cp -av /usr/lib/libgcrypt.so* usr/lib/

rm -vf usr/lib/libgcrypt.so* usr/lib/libxcb-shm.so*

# breaking after .so-version bump
#rm -vf usr/lib/libffi.so* # (process:2633): Gtk-WARNING **: Locale not supported by C library. # has GTK icons regardless of style

# Failed to finding matching FBConfig
#The X11 connection broke: No error (code 0)
#XIO:  fatal IO error 2 (No such file or directory) on X server ":0"
#      after 346 requests (346 known processed) with 0 events remaining.
# Warning: QXcbConnection: XCB error

# libGL error: failed to create drawable
# Unrecognized OpenGL version

#The X11 connection broke: No error (code 0)
#XIO:  fatal IO error 2 (No such file or directory) on X server ":0"
#      after 346 requests (346 known processed) with 0 events remaining.
rm -vf usr/lib/libX11-xcb.so*
#rm -vf usr/lib/libxcb-glx.so*
 # all xcb libraries except input and xinerama are not needed by 1604
#mv -v usr/lib/libxcb-xinput* usr/lib/libxcb-xinerama* .
#rm -vf usr/lib/libxcb*
#mv -v libxcb-xinput* libxcb-xinerama* usr/lib/

mkdir -pv glibc
tar -xf $(find /var/cache/pacman/pkg -iname $(expac "%n-%v-*.pkg.tar.zst" glibc)) -C glibc

set -x
patchelf --add-needed libpthread.so.0 usr/bin/knossos
patchelf --add-needed libQt5XcbQpa.so.5 usr/bin/knossos
patchelf --add-needed libdl.so.2 usr/bin/knossos
cp -v usr/bin/knossos usr/bin/knossos-custom-glibc
patchelf --set-interpreter glibc/usr/lib/ld-linux-x86-64.so.* usr/bin/knossos-custom-glibc
#patchelf --force-rpath --set-rpath "$(patchelf --print-rpath usr/bin/knossos)" usr/bin/knossos # may be needed with newer patchelf versions
set +x

function optional_lib {
	target=$1/$(basename $2)
	shift 1
	mkdir -pv $target
	cp -av "$@" $target
}

for lib in stdc++ gcc_s; do
	optional_lib usr/lib/versioned /usr/lib/lib$lib.so.*
done
for lib in OpenGL GLdispatch GLX GL GLU; do
	optional_lib usr/lib/supplemental /usr/lib/lib$lib.so.*
done
optional_lib usr/lib/supplemental usr/lib/libpython*.so*
rm -vf usr/lib/libpython*.so* # keeping it prioritized will warn about failing »import site«

rm -v AppRun
cp -v ../../knossos/installer/AppRun .

cd ..

time deploy-tools/appimagetool depl
