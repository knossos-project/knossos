#ifndef __TEST_WIDGET_H__
#define __TEST_WIDGET_H__


#include <QWidget>

class TestingWidget : public QWidget {
    Q_OBJECT
    public:
    TestingWidget(QWidget* parent=0);
};

#endif /* __TEST_WIDGET_H__*/