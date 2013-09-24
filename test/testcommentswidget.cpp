#include "testcommentswidget.h"
#include "widgets/widgetcontainer.h"
#include "widgets/mainwindow.h"
#include "widgets/commentswidget.h"
#include "widgets/viewport.h"
#include <QPushButton>
#include <QMessageBox>

extern stateInfo *state;

TestCommentsWidget::TestCommentsWidget(QObject *parent) :
    QObject(parent)
{

}

/** @attention: the upcoming message box is modal and blocks the test method. It has to be queried with yes.**/
void TestCommentsWidget::testDeleteComments() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentShortCutsTab *tab = commentsWidget->shortcutTab;

    commentsWidget->setVisible(true);

    for(int i = 0; i < 5; i++) {
        QTest::keyClicks(tab->textFields[i], QString("A comment %1").arg(i));
    }

   tab->button->clicked(true);

   for(int i = 0; i < 5; i++)
        QCOMPARE(tab->textFields[i]->text(), QString(""));

}

/** This test simply checks if the text in the textfields from F1-F5 is copied into state->viewerState->comment1-5 **/
void TestCommentsWidget::testEnterComments() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentShortCutsTab *tab = commentsWidget->shortcutTab;

    commentsWidget->setVisible(true);

    for(int i = 0; i < 5; i++) {
        QTest::keyClicks(tab->textFields[i], QString("A comment %1").arg(i));
        QTest::keyClick(tab->textFields[i], Qt::Key_Return);

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

/** This test adds a node to the first viewport and change it´s comment via F4
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
    Viewport *firstViewport = reference->vp;

    QPoint pos = firstViewport->pos();
    pos.setX(pos.x() + 10);
    pos.setY(pos.y() + 10);

    QTest::mouseClick(firstViewport, Qt::RightButton, 0,  pos);
    QTest::keyClick(firstViewport, Qt::Key_F4);

    QString usedString = tab->textFields[3]->text();

    QCOMPARE(tools->toolsQuickTabWidget->commentField->text(), usedString);
    QCOMPARE(tools->toolsNodesTabWidget->commentField->text(), usedString);

    bool found;
    // get the id of the node from the quick tab spinbox
    int nodeID = tools->toolsQuickTabWidget->activeNodeSpinBox->value();
    // iterate through the table and find the entry of this node
    for(int k = 0; k < commentsWidget->nodeCommentsTab->nodeTable->rowCount(); k++) {
        QTableWidgetItem *idColumn = commentsWidget->nodeCommentsTab->nodeTable->item(k, 0);
        if(idColumn->text().toInt() == nodeID) {
            QCOMPARE(usedString, commentsWidget->nodeCommentsTab->nodeTable->item(k, 1)->text());
            found = true;
        }
    }

    // the test will fail if no item in the table was found
    QVERIFY(found == true);
}
/*

void TestCommentsWidget::testEnableConditionalColoring() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsPreferencesTab *preferenceTab = reference->window->widgetContainer->commentsWidget->preferencesTab;
    commentsWidget->setVisible(true);



}

void TestCommentsWidget::testEnableConditionalRadius() {
    CommentsWidget *commentsWidget = reference->window->widgetContainer->commentsWidget;
    CommentsPreferencesTab *preferenceTab = reference->window->widgetContainer->commentsWidget->preferencesTab;
    commentsWidget->setVisible(true);

}
*/

