#!/bin/sh
if [ $EUID -ne 0 ]; then
    echo "This script needs to be run as root." > /dev/stderr
    exit 1
fi

# first of all we install the official 2.7.6 distribution 
# the installation process will happen without user interaction
path=$(pwd)

hdiutil mount python-2.7.6.dmg
cd /Volumes/Python\ 2.7.6/
sudo installer -pkg Python.mpkg -target /

# then all dependencies are installed for ipython 2.0
easy_install pip
pip install pyzmq
pip install gnureadline
pip install ipython[all]
cd ..
diskutil umount force Python\ 2.7.6/

# finally knossos will be copied in the application folder
cd $path
hdiutil mount knossos.dmg
cd /Volumes/knossos
cp -rf knossos.app /Applications
cd ..
diskutil umount force knossos
cd ..

echo installation complete
