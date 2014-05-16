%Module(name=pyknossos, version=0)

%Import QtGui/QtGuimod.sip
%Import QtWidgets/QtWidgetsmod.sip

class TestingWidget : QWidget {
%TypeHeaderCode
#include "testwidget.h"
%End

public:
    TestingWidget(QWidget* parent);
};