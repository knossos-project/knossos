#ifndef KNOSSOSTESTRUNNER_H
#define KNOSSOSTESTRUNNER_H

#include <QWidget>
#include <QList>
#include "testcommentswidget.h"
#include "testdatasavingwidget.h"
#include "testnavigationwidget.h"
#include "testorthogonalviewport.h"
#include "testskeletonloadandsave.h"
#include "testskeletonviewport.h"
#include "testtoolswidget.h"
#include "testzoomandmultireswidget.h"
#include "viewer.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPlainTextEdit;
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
    QList<QObject *> *testclassList;
    TestCommentsWidget *testCommentsWidget;
    TestDataSavingWidget *testDataSavingWidget;
    TestNavigationWidget *testNavigationWidget;
    TestOrthogonalViewport *testOrthogonalViewport;
    TestSkeletonViewport *testSkeletonViewport;
    TestToolsWidget *testToolsWidget;
    TestZoomAndMultiresWidget *testZoomAndMultiresWidget;

    QTreeWidgetItem *findItem(const QString &name);
    void addTestClasses();
    QPlainTextEdit *editor;
signals:
    
public slots:
    void startTest();
    void checkResults();
    void testcaseSelected(QTreeWidgetItem *item, int col = 0);
};

#endif // KNOSSOSTESTRUNNER_H
