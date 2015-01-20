# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'knossos_cuber_widgets_log.ui'
#
# Created: Thu Jan 15 14:26:16 2015
#      by: PyQt4 UI code generator 4.11.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_dialog_log(object):
    def setupUi(self, dialog_log):
        dialog_log.setObjectName(_fromUtf8("dialog_log"))
        dialog_log.resize(400, 300)
        self.plain_text_edit_log = QtGui.QPlainTextEdit(dialog_log)
        self.plain_text_edit_log.setGeometry(QtCore.QRect(3, 7, 391, 281))
        self.plain_text_edit_log.setObjectName(_fromUtf8("plain_text_edit_log"))

        self.retranslateUi(dialog_log)
        QtCore.QMetaObject.connectSlotsByName(dialog_log)

    def retranslateUi(self, dialog_log):
        dialog_log.setWindowTitle(_translate("dialog_log", "Cubing Log", None))

