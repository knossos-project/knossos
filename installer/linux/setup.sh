#!/bin/sh
if [ $EUID -ne 0 ]; then
    echo "This script needs to be run as root." > /dev/stderr
    exit 1
fi

#apt-get takes care about missing dependencies
apt-get -y install libpython2.7-dev python-setuptools python-pip

pip install pyzmq
pip install gnureadline
pip install ipython[all]

dpkg -i knossos.deb

# todo the menu entry from the debian packages can be moved to that place here.
