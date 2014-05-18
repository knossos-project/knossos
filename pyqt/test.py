#! /usr/bin/env python3

from pyknossos import TestingWidget
from PyQt5.QtWidgets import QApplication
import sys

app = QApplication(sys.argv)
w = TestingWidget(None)
w.show()
app.exec_()
