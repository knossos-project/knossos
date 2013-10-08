#ifndef TESTDATASAVINGWIDGET_H
#define TESTDATASAVINGWIDGET_H

#include <QObject>

class Viewer;
class TestDataSavingWidget : public QObject
{
    Q_OBJECT
public:
    explicit TestDataSavingWidget(QObject *parent = 0);
    Viewer *reference;
signals:
    
public slots:
    
};

#endif // TESTDATASAVINGWIDGET_H
