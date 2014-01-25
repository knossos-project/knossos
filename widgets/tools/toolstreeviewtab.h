#ifndef TREEVIEWTAB_H
#define TREEVIEWTAB_H

#include <QWidget>
#include <QTableWidget>

class QTableWidgetItem;
class QHBoxLayout;
class QVBoxLayout;
class QLineEdit;
class QLabel;
class QRadioButton;
class QCheckBox;
class QPushButton;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QSplitter;
class QMenu;

struct treeListElement;
struct nodeListElement;
struct segmentListElement;
class TreeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit TreeTable(QWidget *parent);
    int droppedOnTreeID;
    bool changeByCode;
    void setItem(int row, int column, QTableWidgetItem *item);
protected:
    void keyPressEvent(QKeyEvent *event);
    void dropEvent(QDropEvent *event);
    void focusInEvent(QFocusEvent *);
signals:
    void focused(TreeTable *table);
    void updateTreeview();
    void deleteTreesSignal();
public slots:
};

class NodeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit NodeTable(QWidget *parent);

    bool changeByCode;
    void setItem(int row, int column, QTableWidgetItem *item);
protected:
    void keyPressEvent(QKeyEvent *event);
    void focusInEvent(QFocusEvent *);
signals:
    void deleteNodesSignal();
    void nodesDeletedSignal(QModelIndexList selected);
    void updateNodesTable();
    void focused(NodeTable *table);
public slots:

};

class ToolsTreeviewTab : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreeviewTab(QWidget *parent = 0);

    TreeTable *activeTreeTable;
    TreeTable *treeTable;
    TreeTable *focusedTreeTable; // holds activeTreeTable or treeTable depending on what is focused by the user
    NodeTable *activeNodeTable;
    NodeTable *nodeTable;
    NodeTable *focusedNodeTable;

    QLineEdit *treeSearchField;
    QLineEdit *nodeSearchField;
    QCheckBox *treeRegExCheck;
    QCheckBox *nodeRegExCheck;
    QRadioButton *nodesOfSelectedTreesRadio;
    QRadioButton *allNodesRadio;
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
    // tree action dialogs
    // tree comment editing
    QDialog *treeCommentEditDialog;
    QLineEdit *treeCommentField;
    QPushButton *treeApplyButton;
    QPushButton *treeCancelButton;
    QVBoxLayout *treeCommentLayout;
    QString treeCommentBuffer;
    // tree color editing
    QDialog *treeColorEditDialog;
    int treeColorEditRow;
    QLabel *rLabel, *gLabel, *bLabel, *aLabel;
    QDoubleSpinBox *rSpin, *gSpin, *bSpin, *aSpin;
    QPushButton *treeColorApplyButton;
    QPushButton *treeColorCancelButton;

    // node action dialogs
    QDialog *nodeCommentEditDialog;
    QLineEdit *nodeCommentField;
    QPushButton *nodeCommentApplyButton;
    QPushButton *nodeCommentCancelButton;
    QVBoxLayout *nodeCommentLayout;
    QString nodeCommentBuffer;
    QDialog *nodeRadiusEditDialog;
    QLabel *radiusLabel;
    QDoubleSpinBox *radiusSpin;
    QPushButton *nodeRadiusApplyButton;
    QPushButton *nodeRadiusCancelButton;
    QVBoxLayout *nodeRadiusLayout;
    QDialog *moveNodesDialog;
    QLabel *newTreeLabel;
    QSpinBox *newTreeIDSpin;
    QPushButton *moveNodesButton;
    QPushButton *moveNodesCancelButton;

    float radiusBuffer;

    // drag'n drop buffers
    int draggedNodeID;
    int displayedNodes;

    void updateTreeColorCell(TreeTable *table, int row);
    bool matchesSearchString(QString searchString, QString string, bool useRegEx);

    void createTreesContextMenu();
    void createNodesContextMenu();
    void createContextMenuDialogs();
    QPushButton *confirmationPrompt(QString question, QString confirmString);

protected:
    void insertTree(treeListElement *tree, TreeTable *table);
    void insertNode(nodeListElement *node, NodeTable *table);
    void setText(TreeTable *table, QTableWidgetItem *item, QString text);
    void setText(NodeTable *table, QTableWidgetItem *item, QString text);
    int getActiveTreeRow();
    int getActiveNodeRow();
signals:
    void updateListedNodesSignal(int n);
    void updateToolsSignal();
    void deleteSelectedTreesSignal();
    void delActiveNodeSignal();
    void deleteSelectedNodesSignal();
    void setActiveNodeSignal(int revision, nodeListElement *node, int nodeID);
    void JumpToActiveNodeSignal();
    bool addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();

    void displayedNodesChanged(int index);
    void setFocused(TreeTable *table);
    void setFocused(NodeTable *table);
    void actTreeItemChanged(QTableWidgetItem *item);
    void activeTreeSelected();
    void treeItemChanged(QTableWidgetItem* item);
    void treeItemSelected();
    void treeItemDoubleClicked(QTableWidgetItem* item);
    void actNodeItemChanged(QTableWidgetItem *item);
    void nodeItemChanged(QTableWidgetItem* item);
    void activeNodeSelected();
    void nodeItemSelected();
    void nodeItemDoubleClicked(QTableWidgetItem*);
    // tree context menu
    void treeContextMenuCalled(QPoint pos);
    void setActiveTreeAction();
    void editTreeColor();
    void deleteTreesAction();
    void mergeTreesAction();
    void restoreColorAction();
    void setTreeCommentAction();
    void updateTreeCommentBuffer(QString comment);
    void editTreeComments();

    // node context menu
    void nodeContextMenuCalled(QPoint pos);
    void setNodeRadiusAction();
    void linkNodesAction();
    void moveNodesAction();
    void splitComponentAction();
    void setNodeCommentAction();
    void updateNodeCommentBuffer(QString comment);
    void editNodeComments();
    void updateNodeRadiusBuffer(double value);
    void editNodeRadii();
    void deleteNodesAction();
    void moveNodesClicked();

    // update tree table
    void treeActivated();
    void treeAdded(treeListElement *tree);
    void treesDeleted();
    void treesMerged(int treeID1, int treeID2);
    void treeComponentSplit();

    // update node table
    void nodeActivated();
    void nodeAdded();
    void nodesDeleted();
    void branchPushed();
    void branchPopped();
    void nodeCommentChanged(nodeListElement *node);
    void nodeRadiusChanged(nodeListElement *node);
    void nodePositionChanged(nodeListElement *node);
    void updateTreesTable();
    void updateNodesTable();
    void update();
    
};

#endif // TREEVIEWTAB_H
