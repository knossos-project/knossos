#include "testcommentswidget.h"
#include "widgets/widgetcontainer.h"
#include "widgets/mainwindow.h"
#include "widgets/commentswidget.h"
#include "widgets/viewport.h"
#include "widgets/toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "QtOpenGL/qgl.h"
#include <QPushButton>
#include <QMessageBox>

extern stateInfo *state;

TestCommentsWidget::TestCommentsWidget(QObject *parent) :
    QObject(parent)
{

}

void TestCommentsWidget::testDeleteComments() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentShortCutsTab *tab = commentsWidget->shortcutTab;

    commentsWidget->setVisible(true);

    for(int i = 0; i < 5; i++) {
        QTest::keyClicks(tab->textFields[i], QString("A comment %1").arg(QString::number(i)));
    }

   tab->deleteCommentsWithoutConfirmation();

   for(int i = 0; i < 5; i++)
        QCOMPARE(tab->textFields[i]->text(), QString(""));

   reference->window->clearSkeletonWithoutConfirmation();

}

/** This test simply checks if the text in the textfields from F1-F5 is copied into state->viewerState->comment1-5 **/
void TestCommentsWidget::testEnterComments() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentShortCutsTab *tab = commentsWidget->shortcutTab;

    commentsWidget->setVisible(true);

    for(int i = 0; i < 5; i++) {
        QTest::keyClicks(tab->textFields[i], QString("A comment %1").arg(QString::number(i)));

        if(i == 0) {
            QCOMPARE(QString(state->viewerState->gui->comment1), tab->textFields[i]->text());
        } else if(i == 1) {
            QCOMPARE(QString(state->viewerState->gui->comment2), tab->textFields[i]->text());
        } else if(i == 2) {
             QCOMPARE(QString(state->viewerState->gui->comment3), tab->textFields[i]->text());
        } else if(i == 3) {
             QCOMPARE(QString(state->viewerState->gui->comment4), tab->textFields[i]->text());
        } else if(i == 4) {
             QCOMPARE(QString(state->viewerState->gui->comment5), tab->textFields[i]->text());
        }
    }
}

/** This test adds a node to the first viewport and change it´s comment via the F1-F5 Buttons
 *  Three Widget locations will be checked.
 *  - 1. ToolsQuickTab commentsField
 *  - 2. ToolsNodeTab commentsField
 *  - 3. NodeCommentsTab the string value of the second column in the table
 */
void TestCommentsWidget::testAddNodeComment() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentShortCutsTab *tab = commentsWidget->shortcutTab;
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;

    commentsWidget->setVisible(true);
    Viewport *firstViewport = reference->vpUpperLeft;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    for(int i = 0; i < 5; i++) {
        QTest::mouseClick(firstViewport, Qt::RightButton, 0,  pos);

        if(i == 0)
            QTest::keyClick(firstViewport, Qt::Key_F1);
        else if(i == 1)
            QTest::keyClick(firstViewport, Qt::Key_F2);
        else if(i == 2)
            QTest::keyClick(firstViewport, Qt::Key_F3);
        else if(i == 3)
            QTest::keyClick(firstViewport, Qt::Key_F4);
        else if(i == 4)
            QTest::keyClick(firstViewport, Qt::Key_F5);


        QString usedString = tab->textFields[i]->text();
        qDebug() << usedString << "_" << tools->toolsQuickTabWidget->commentField->text() << "_" << tools->toolsNodesTabWidget->commentField->text();

        QCOMPARE(tools->toolsQuickTabWidget->commentField->text(), usedString);
        QCOMPARE(tools->toolsNodesTabWidget->commentField->text(), usedString);


        bool found;
        // get the id of the node from the quick tab spinbox
        int nodeID = tools->toolsQuickTabWidget->activeNodeSpinBox->value();
        // iterate through the table and find the entry of this node


        for(int k = 0; k < commentsWidget->nodeCommentsTab->nodeTable->rowCount(); k++) {
            QTableWidgetItem *idColumn = commentsWidget->nodeCommentsTab->nodeTable->item(k, 0);
            if(idColumn) {
                if(idColumn->text().toInt() == nodeID) {
                    QCOMPARE(usedString, commentsWidget->nodeCommentsTab->nodeTable->item(k, 1)->text());
                    found = true;
                }
            }
        }

    // the test will fail if no item in the table was found
    QVERIFY(found == true);
    }

    reference->window->clearSkeletonWithoutConfirmation();
}

/** This testcase declares a tree and two nodes
*  A random color from the combobox will be selected, a matching substring will be defined and the conditional coloring enabled
*  Then the resulting color will be checked
*/
void TestCommentsWidget::testEnableConditionalColoring() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsHighlightingTab *highlightingTab = reference->window->widgetContainer->commentsWidget->highlightingTab;
    commentsWidget->setVisible(true);

    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    // the first node of a tree is a branchnode which won´t be colored
    QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
    // that´s why another node is needed

    for(int i = 0; i < 5; i++) {
        QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
        // we need a comment to check the coloring later
        reference->window->widgetContainer->toolsWidget->toolsQuickTabWidget->commentLabel->setText("Test");
        // the coloring checkbox will be enabled and a corresponding substring will be defined

        int random = i /*rand() % 5 */;

        highlightingTab->substringFields[i]->setText("T");
        highlightingTab->colorComboBox[i]->currentIndexChanged(random);

        if(!highlightingTab->enableCondColoringCheckBox->isChecked())
            highlightingTab->enableCondColoringCheckBox->setChecked(true);
        else {
            QCOMPARE(state->skeletonState->userCommentColoringOn, highlightingTab->enableCondColoringCheckBox->isChecked());
        }

        // check the color declared in the skeleton Model
        color4F *color = state->skeletonState->commentColors;


        if(random == 0) {
            // the reference values are 0.13, 0.69, 0.3, 1.                        
            QVERIFY(color->r == 0.13);
            QVERIFY(color->g == 0.69);
            QVERIFY(color->b == 0.3);
            QVERIFY(color->a == 1.F);
        } else if(random == 1) {
            // the reference values are 0.94, 0.89, 0.69, 1.
            QVERIFY(color->r == 0.94);
            QVERIFY(color->g == 0.89);
            QVERIFY(color->b == 0.69);
            QVERIFY(color->a == 1.F);
        } else if(random == 2) {
            // the reference values are 0.6, 0.85, 0.92, 1.)            
            QVERIFY(color->r == 0.6);
            QVERIFY(color->g == 0.85);
            QVERIFY(color->b == 0.92);
            QVERIFY(color->a == 1.F);
        } else if(random == 3) {
            // the reference values are 0.64, 0.29, 0.64, 1.);
            QVERIFY(color->r == 0.64);
            QVERIFY(color->g == 0.29);
            QVERIFY(color->b == 0.64);
            QVERIFY(color->a == 1.F);
        } else if(random == 4) {
            // the reference values are 0.73, 0.48, 0.34, 1.
            QVERIFY(color->r == 0.73F);
            QVERIFY(color->g == 0.48F);
            QVERIFY(color->b == 0.34F);
            QVERIFY(color->a == 1.F);
        }
    }

    reference->window->clearSkeletonWithoutConfirmation();

}

/** This testcase checks the conditional radius */
void TestCommentsWidget::testEnableConditionalRadius() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsHighlightingTab *highlightingTab = reference->window->widgetContainer->commentsWidget->highlightingTab;
    commentsWidget->setVisible(true);

    Viewport *firstViewport = reference->vpUpperLeft;
    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    int random[] = {10, 20, 50, 80, 100};

    for(int i = 0; i < 5; i++) {
        // the first node of a tree is a branchnode which won´t be colored
        QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
        // that´s why another node is needed
        QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
        // we need a comment to check the coloring later
        reference->window->widgetContainer->toolsWidget->toolsQuickTabWidget->commentField->setText("Test");

        // the coloring checkbox will be enabled and a corresponding substring will be defined
        highlightingTab->substringFields[i]->setText("T");

        highlightingTab->radiusSpinBox[i]->setValue(random[i]);
        if(!highlightingTab->enableCondRadiusCheckBox->isChecked())
            highlightingTab->enableCondRadiusCheckBox->setChecked(true);

        // check the resulting radius size
        QVERIFY(random[i] == state->skeletonState->commentNodeRadii[i]);
    }

    reference->window->clearSkeletonWithoutConfirmation();
}


/** @tood this testcase needs a more complex rework */
void TestCommentsWidget::testCommentsTable() {

    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsNodeCommentsTab *nodeCommentsTab = reference->window->widgetContainer->commentsWidget->nodeCommentsTab;
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    // lets add a couple of nodes, remember that the first node is a branchnode
    for(int i = 0; i < 5; i++) {
        QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
        if(i > 0) {
            tools->toolsQuickTabWidget->commentField->setText("Random");
            QTest::keyClick(tools->toolsQuickTabWidget->commentField, Qt::Key_Return);
        }
    }

    // check if branchnodes are filtered
    commentsWidget->nodeCommentsTab->branchPointOnlyChecked(true);
    int value = commentsWidget->nodeCommentsTab->nodeTable->rowCount();
    // only one node should be in the table the branchnode (id = 1)
    QCOMPARE(1, commentsWidget->nodeCommentsTab->nodeTable->rowCount());
    commentsWidget->nodeCommentsTab->branchPointOnlyChecked(false);

    // check if the substring filtering works
    commentsWidget->nodeCommentsTab->filterField->setText("R");
    QTest::keyClick(commentsWidget->nodeCommentsTab->filterField, Qt::Key_Return);
    // all nodes should be listed excepted the first branchnode (id = 1)
    QCOMPARE(4, commentsWidget->nodeCommentsTab->nodeTable->rowCount());

    commentsWidget->nodeCommentsTab->filterField->setText("");
    QTest::keyClick(commentsWidget->nodeCommentsTab->filterField, Qt::Key_Return);

    // the active node id is 5 at the moment, lets delete a node
    QTest::keyClick(firstViewport, Qt::Key_Delete);
    QCOMPARE(3, commentsWidget->nodeCommentsTab->nodeTable->rowCount());

    reference->window->clearSkeletonWithoutConfirmation();
}

/** @todo this testcase exceeds the maximum amount of warnings.
 *The parameter -maxwarnings should be 0 for unlimited warning.
 Have to find out if it is a compile time or a runtime argument and where ..*/
void TestCommentsWidget::testCommentsTablePerformance() {

    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsNodeCommentsTab *nodeCommentsTab = reference->window->widgetContainer->commentsWidget->nodeCommentsTab;
    ToolsWidget *tools = reference->window->widgetContainer->toolsWidget;
    Viewport *firstViewport = reference->vpUpperLeft;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    int n = 1000;
    QTime bench;
    int msec;
    // lets add n nodes each having a comment
    for(int i = 0; i <= n; i++) {
        if(i == n-1) {
            bench.start();
        }
        QTest::mouseClick(firstViewport, Qt::RightButton, 0, pos);
        tools->toolsQuickTabWidget->commentField->setText("Random Text");
        QTest::keyClick(tools->toolsQuickTabWidget->commentField, Qt::Key_Return);

        if(i == n-1) {
            msec = bench.elapsed();
        }
    }

    QTextEdit edit;
    edit.setText(QString("%1 msec").arg(msec));
    edit.show();

    reference->window->clearSkeletonWithoutConfirmation();
}

void TestCommentsWidget::cleanupTestCase() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    commentsWidget->hide();
    emit ready();
}
