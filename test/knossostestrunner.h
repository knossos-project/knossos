#ifndef KNOSSOSTESTRUNNER_H
#define KNOSSOSTESTRUNNER_H

#include <QWidget>
#include "testcommentswidget.h"
#include "viewer.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class KnossosTestRunner : public QWidget
{
    Q_OBJECT
public:
    explicit KnossosTestRunner(QWidget *parent = 0);
    QTreeWidget *treeWidget;
    QLabel *passLabel;
    QLabel *failLabel;
    int pass, fail, total;

    Viewer *reference;
    TestCommentsWidget *testCommentsWidget;

    QTreeWidgetItem *findItem(const QString &name);
    void addTestClasses();

signals:
    
public slots:
    void startTest();
    void checkResults();
};

#endif // KNOSSOSTESTRUNNER_H
