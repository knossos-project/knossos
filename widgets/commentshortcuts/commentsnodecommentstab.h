#ifndef COMMENTSNODECOMMENTSTAB_H
#define COMMENTSNODECOMMENTSTAB_H

#include <QWidget>
#include <QList>
#include "knossos-global.h"

class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class CommentsNodeCommentsTab : public QWidget
{
    friend class TestCommentsWidget;
    Q_OBJECT
public:
    explicit CommentsNodeCommentsTab(QWidget *parent = 0);    
signals:
    void updateCommentsTableSignal();
    void setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void setJumpToActiveNodeSignal();
    nodeListElement *findNodeByNodeIDSignal(int id);
    void updateTools();
public slots:

public slots:
    void updateCommentsTable();
    void branchPointOnlyChecked(bool on);
    void filterChanged();
    void commentChanged(QTableWidgetItem *item);
    void itemSelected(int row, int col);
    void doubleClicked(QTableWidgetItem *item = 0);

protected:
    QCheckBox *branchNodesOnlyCheckbox;
    QLabel *filterLabel;
    QLineEdit *filterField;    
    QTableWidget *nodeTable;
    int tableIndex;
    QList<QString> *nodesWithComment;

};

#endif // COMMENTSNODECOMMENTSTAB_H
