#! /usr/bin/python3

from __future__ import print_function

from PyQt5.QtCore import PYQT_CONFIGURATION

x = PYQT_CONFIGURATION['sip_flags']
print("pyqt_sip_flags:%s" % x)
