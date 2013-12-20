#include "testtoolswidget.h"
#include "widgets/mainwindow.h"
#include "widgets/widgetcontainer.h"
#include "widgets/toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/viewport.h"
#include "sleeper.h"

extern stateInfo *state;

TestToolsWidget::TestToolsWidget(QObject *parent) :
    QObject(parent)
{
}

/* A random amount of trees are added with the newTreeButton.
 * This testcase checks if the information of the skeleton is the same
 * with the displayed information
 */
void TestToolsWidget::testAddTreesPerMouseClick() {
    // first a number of random trees is chosen
    int random = rand() % 100 + 1;

    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;

    for(int i = 0; i < random; i++) {
        tools->toolsTreesTabWidget->newTreeButton->click();
    }

    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    QCOMPARE(random, state->skeletonState->treeElements);
    QCOMPARE(random, tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt());
    QCOMPARE(random, tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt());

    reference->window->clearSkeletonWithoutConfirmation();

}

/* A random amount of trees are added with pushing Key_C.
 * This testcase checks if the information of the skeleton is the same
 * with the displayed information
 */
void TestToolsWidget::testAddTreesPerKeyPress() {

    //first we define a random amount of trees per viewport
    int random[3];

    for(int i = 0; i < 3; i++) {
        random[i] = rand() % 100 + 1;
    }

    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;
    Viewport *secondViewport = reference->vpLowerLeft;
    Viewport *thirdViewport = reference->vpUpperRight;

    // first viewport
    // then the random amount of trees are added
    for(int i = 0; i < random[0]; i++) {       
        QTest::keyClick(firstViewport, Qt::Key_C);
    }

    int trees = state->skeletonState->treeElements;
    QCOMPARE(random[0], trees);

    // the active Tree ID is up to date if a node is added
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTextEdit *textEdit = new QTextEdit();
    textEdit->insertPlainText("Testoutput for TestToolsWidget\n");
    textEdit->insertPlainText(QString("%1:%2\n").arg(pos.x()).arg(pos.y()));

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    int activeTree = tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(random[0], activeTree);
    activeTree = tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(random[0], activeTree);

    // next viewport
    for(int i = 0; i < random[1]; i++) {
        QTest::keyClick(secondViewport, Qt::Key_C);
    }

    trees = state->skeletonState->treeElements;
    QCOMPARE(trees, random[0] + random[1]);

    // the active Tree ID is up to date if a node is added
    pos = secondViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    textEdit->append(QString("%1:%2").arg(pos.x()).arg(pos.y()));
    QTest::mouseClick(secondViewport, Qt::RightButton, 0, pos);

    QCOMPARE(trees, random[0] + random[1]);

    activeTree = tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(activeTree, random[0] + random[1]);
    activeTree = tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(activeTree, random[0] + random[1]);

    // next viewport
    for(int i = 0; i < random[2]; i++) {
        QTest::keyClick(thirdViewport, Qt::Key_C);
    }

    trees = state->skeletonState->treeElements;
    QCOMPARE(trees, random[0] + random[1] + random[2]);

    // The active tree is first changed if a node is added
    pos = thirdViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(thirdViewport, Qt::RightButton, 0, pos);

    QCOMPARE(trees, random[0] + random[1] + random[2]);

    activeTree = tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(activeTree, random[0] + random[1] + random[2]);
    activeTree = tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt();
    QCOMPARE(activeTree, random[0] + random[1] + random[2]);


    reference->window->clearSkeletonWithoutConfirmation();
}

/* This testcase adds an amount of nodes with the right mouse button and compare the
 skeleton model with the displayed information */
void TestToolsWidget::testAddNodesPerMouseClick() {
    // first we define a random amount of nodes per viewport
    int random[3];

    for(int i = 0; i < 3; i++) {
        random[i] = rand() % 1000 + 1;
    }

    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;
    Viewport *secondViewport = reference->vpLowerLeft;
    Viewport *thirdViewport = reference->vpUpperRight;

    // first viewport
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    // then the random amount of nodes are added
    for(int i = 0; i < random[0]; i++) {
        QTest::mouseClick(firstViewport, Qt::RightButton, 0,  pos);
    }

    // the amount of added nodes should be the same as in the following ui elements
    int addedNodes = state->skeletonState->totalNodeElements;
    QCOMPARE(addedNodes, random[0]);

    int activeNode = tools->toolsQuickTabWidget->activeNodeSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0]);

    activeNode = tools->toolsNodesTabWidget->activeNodeIdSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0]);

    // second viewport
    pos = secondViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    // then the random amount of nodes are added
    for(int i = 0; i < random[1]; i++) {
        QTest::mouseClick(secondViewport, Qt::RightButton, 0, pos);
    }

    // again the amount of nodes should be the same as in the following ui elements
    addedNodes = state->skeletonState->totalNodeElements;
    QCOMPARE(addedNodes, random[0] + random[1]);

    activeNode = tools->toolsQuickTabWidget->activeNodeSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0] + random[1]);

    activeNode = tools->toolsNodesTabWidget->activeNodeIdSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0] + random[1]);

    // third viewport
    pos = thirdViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    // then the random amount of nodes are added
    for(int i = 0; i < random[2]; i++) {
        QTest::mouseClick(thirdViewport, Qt::RightButton, 0, pos);
    }

    // again the amount of nodes should be the same as in the following ui elements
    addedNodes = state->skeletonState->totalNodeElements;
    QCOMPARE(addedNodes, random[0] + random[1] + random[2]);

    activeNode = tools->toolsQuickTabWidget->activeNodeSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0] + random[1] + random[2]);

    activeNode = tools->toolsNodesTabWidget->activeNodeIdSpinBox->text().toInt();
    QCOMPARE(activeNode, random[0] + random[1] + random[2]);

    reference->window->clearSkeletonWithoutConfirmation();
}

/* This test case checks if all skeleton information are cleared after using the DeleteActiveTreeButton */
void TestToolsWidget::testDeleteActiveTreeCaseZero() {
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    QCOMPARE(state->skeletonState->treeElements, tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt());
    QCOMPARE(state->skeletonState->treeElements, tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt());

    tools->toolsTreesTabWidget->deleteActiveTreeWithoutConfirmation();

    /* After pushing the DeleteActiveNodeButton activeTreeID and activeNodeID should be zero */
    QVERIFY(state->skeletonState->treeElements == 0);
    QCOMPARE(state->skeletonState->treeElements, tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt());
    QCOMPARE(state->skeletonState->treeElements, tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt());

    QVERIFY(state->skeletonState->totalNodeElements == 0);
    QCOMPARE(state->skeletonState->totalNodeElements, tools->toolsQuickTabWidget->activeNodeSpinBox->text().toInt());
    QCOMPARE(state->skeletonState->totalNodeElements, tools->toolsNodesTabWidget->activeNodeIdSpinBox->text().toInt());

    reference->window->clearSkeletonWithoutConfirmation();
}


void TestToolsWidget::testDeleteActiveTreeCaseNotZero() {
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;

    // lets add a tree and at least two nodes
    //QTest::keyClick(firstViewport, Qt::Key_C);

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    // lets add another tree and one node
    QTest::keyClick(firstViewport, Qt::Key_C);
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    // now we delete the last tree and check the node ID
    tools->toolsTreesTabWidget->deleteActiveTreeWithoutConfirmation();

    QCOMPARE(QString("Tree Count: 1"), tools->toolsQuickTabWidget->treeCountLabel->text());
    QCOMPARE(QString("Node Count: 2"), tools->toolsQuickTabWidget->nodeCountLabel->text());

    reference->window->clearSkeletonWithoutConfirmation();

    // Another Case

    // lets add n Trees
    int random = rand() % 100 + 1;
    for(int i = 0; i < random; i++) {
        QTest::keyClick(firstViewport, Qt::Key_C);
    }

    pos = firstViewport->pos();
    // and a node for tree no n
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos, 50);

    // now we delete the last tree and check the node ID
    tools->toolsTreesTabWidget->deleteActiveTreeWithoutConfirmation();


    QCOMPARE(0 ,tools->toolsQuickTabWidget->activeNodeSpinBox->text().toInt());
    QCOMPARE(0 ,tools->toolsNodesTabWidget->activeNodeIdSpinBox->text().toInt());


    reference->window->clearSkeletonWithoutConfirmation();
}

void TestToolsWidget::testMergeTrees() {
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;

    // lets create 2 trees with 1 node
    //QTest::keyClick(firstViewport, Qt::Key_C);

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
    QTest::keyClick(firstViewport, Qt::Key_C);
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    tools->toolsTreesTabWidget->id1SpinBox->setValue(1);
    tools->toolsTreesTabWidget->id2SpinBox->setValue(2);
    // lets push the merge button
    tools->toolsTreesTabWidget->mergeTreesButton->click();

    QVERIFY(1 == state->skeletonState->treeElements);
    QVERIFY(1 == tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt());
    QVERIFY(1 == tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt());

    reference->window->clearSkeletonWithoutConfirmation();
}

void TestToolsWidget::testSplitConnectedComponents() {
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    // Let´s change the workmode to a
    QTest::keyClick(firstViewport, Qt::Key_A);
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);

    tools->toolsTreesTabWidget->splitByConnectedComponentsButtonClicked();

    int trees = state->skeletonState->treeElements;

    QVERIFY(2 == state->skeletonState->treeElements);
    QVERIFY(2 == tools->toolsQuickTabWidget->activeTreeSpinBox->text().toInt());
    QVERIFY(2 == tools->toolsTreesTabWidget->activeTreeSpinBox->text().toInt());

    reference->window->clearSkeletonWithoutConfirmation();
}

void TestToolsWidget::testRestoreColors() {
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;

    int r = rand() % 100;
    int g = rand() % 100;
    int b = rand() % 100;
    int a = rand() % 100;

    tools->toolsTreesTabWidget->rSpinBox->setValue(r);
    tools->toolsTreesTabWidget->gSpinBox->setValue(g);
    tools->toolsTreesTabWidget->bSpinBox->setValue(b);
    tools->toolsTreesTabWidget->aSpinBox->setValue(a);

    tools->toolsTreesTabWidget->restoreDefaultColorButtonClicked();

    QCOMPARE(tools->toolsTreesTabWidget->rSpinBox->value(), 0.0);
    QCOMPARE(tools->toolsTreesTabWidget->gSpinBox->value(), 0.0);
    QCOMPARE(tools->toolsTreesTabWidget->bSpinBox->value(), 0.0);
    QCOMPARE(tools->toolsTreesTabWidget->aSpinBox->value(), 0.0);

    reference->window->clearSkeletonWithoutConfirmation();
}
