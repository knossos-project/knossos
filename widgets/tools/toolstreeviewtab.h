#ifndef TREEVIEWTAB_H
#define TREEVIEWTAB_H

#include <QWidget>
#include <QRadioButton>
#include <QTableWidget>

#include "widgets/tools/nodetable.h"
#include "widgets/tools/treetable.h"

class QTableWidgetItem;
class QHBoxLayout;
class QVBoxLayout;
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QSplitter;
class QMenu;

class treeListElement;
class nodeListElement;
class segmentListElement;

class ToolsTreeviewTab : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreeviewTab(QWidget *parent = 0);

    TreeTable *activeTreeTable;
    TreeTable *treeTable;
    NodeTable *activeNodeTable;
    NodeTable *nodeTable;

    QLineEdit *treeSearchField;
    QLineEdit *nodeSearchField;
    QCheckBox *treeRegExCheck;
    QCheckBox *nodeRegExCheck;
    QRadioButton *nodesOfSelectedTreesRadio;
    QRadioButton *allNodesRadio;
    QRadioButton selectedNodes;//i wanna be special

    QCheckBox *branchNodesChckBx;
    QCheckBox *commentNodesChckBx;
    QLabel *displayedNodesTable;
    QComboBox *displayedNodesCombo;
    QMenu *treeContextMenu;
    QMenu *nodeContextMenu;

    QSplitter *splitter;
    QWidget *treeSide;
    QWidget *nodeSide;
    QVBoxLayout *mainLayout;

    //edit dialogs values
    QString nodeCommentBuffer;
    float radiusBuffer;
    QString treeCommentBuffer;

    // drag'n drop buffers
    int draggedNodeID;
    int displayedNodes;

    void updateTreeColorCell(TreeTable *table, int row);
    bool matchesSearchString(QString searchString, QString string, bool useRegEx);

    QPushButton *confirmationPrompt(QString question, QString confirmString);

protected:
    void insertTree(treeListElement *tree, TreeTable *table);
    void insertNode(nodeListElement *node, NodeTable *table);
    void setText(TreeTable *table, QTableWidgetItem *item, QString text);
    void setText(NodeTable *table, QTableWidgetItem *item, QString text);
    int getActiveTreeRow();
    int getActiveNodeRow();
signals:
    void updateAnnotationLabelsSignal();
    void deleteSelectedTreesSignal();
    void delActiveNodeSignal();
    void deleteSelectedNodesSignal();
    void clearTreeSelectionSignal();
    void clearNodeSelectionSignal();
    void setActiveNodeSignal(int revision, nodeListElement *node, int nodeID);
    void JumpToActiveNodeSignal();
    bool addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();

    void displayedNodesChanged(int index);
    void actTreeItemChanged(QTableWidgetItem *item);
    void activeTreeSelectionChanged();
    void treeItemChanged(QTableWidgetItem* item);
    void treeSelectionChanged();
    void treeItemDoubleClicked(QTableWidgetItem* item);

    void actNodeItemChanged(QTableWidgetItem *item);
    void nodeItemChanged(QTableWidgetItem* item);
    void activeNodeSelectionChanged();
    void nodeSelectionChanged();
    void nodeItemDoubleClicked(QTableWidgetItem*);
    void sortComments(const int);

    void activateFirstSelectedNode();
    void activateFirstSelectedTree();

    //context menu
    void contextMenu(QPoint pos);
    // tree context menu
    void deleteTreesAction();
    void mergeTreesAction();
    void restoreColorAction();
    void setTreeCommentAction();
    // node context menu
    void setNodeRadiusAction();
    void linkNodesAction();
    void moveNodesAction();
    void splitComponentAction();
    void setNodeCommentAction();
    void deleteNodesAction();

    // update tree table
    void recreateTreesTable();
    void treeActivated();
    void treeAdded(treeListElement *tree);
    void treesDeleted();
    void treesMerged(int treeID1, int treeID2);

    // update node table
    void clearNodeTableSelection();
    void recreateNodesTable();
    void nodeActivated();
    void nodeAdded();
    void branchPushed();
    void branchPopped();
    void nodeCommentChanged(nodeListElement *node);
    void nodeRadiusChanged(nodeListElement *node);
    void nodePositionChanged(nodeListElement *node);

    void update();
};

#endif // TREEVIEWTAB_H
