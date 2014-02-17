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
class Patch;

class KTable : public QTableWidget {
    Q_OBJECT
public:
    explicit KTable(QWidget *parent);
    bool changeByCode;
    void setItem(int row, int column, QTableWidgetItem *item);
    void keyPressed(QKeyEvent *event);
protected:
    void focusInEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *event);
signals:
    void focused(KTable *table);
    void deleteSignal();
public slots:
};

class TreeTable : public KTable {
    Q_OBJECT
public:
    explicit TreeTable(QWidget *parent);
    int droppedOnTreeID;

protected:
    void dropEvent(QDropEvent *event);

signals:

public slots:
};

class ToolsTreeviewTab : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreeviewTab(QWidget *parent = 0);

    TreeTable *activeTreeTable;
    TreeTable *treeTable;
    KTable *activeNodeTable;
    KTable *nodeTable;
    KTable *activePatchTable;
    KTable *patchTable;
    KTable *focusedTable; //! holds one of the existing table

    QLineEdit *treeSearchField;
    QLineEdit *nodeSearchField;
    QLineEdit *patchSearchField;
    QCheckBox *treeRegExCheck;
    QCheckBox *nodeRegExCheck;
    QCheckBox *patchRegExCheck;
    QRadioButton *nodesOfSelectedTreesRadio;
    QRadioButton *allNodesRadio;
    QCheckBox *branchNodesChckBx;
    QCheckBox *commentNodesChckBx;
    QLabel *displayedNodesTable;
    QComboBox *displayedNodesCombo;
    QMenu *treeContextMenu;
    QMenu *nodeContextMenu;
    QMenu *patchContextMenu;

    QSplitter *splitter;
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

    void createContextMenus();
    void createContextMenuDialogs();
    QPushButton *confirmationPrompt(QString question, QString confirmString);

protected:
    void insertTree(treeListElement *tree, TreeTable *table);
    void insertNode(nodeListElement *node, KTable *table);
    void insertPatch(Patch *patch, KTable *table);
    void setText(KTable *table, QTableWidgetItem *item, QString text);
    int getActiveTreeRow();
    int getActiveNodeRow();
signals:
    void updateListedNodesSignal(int n);
    void updateToolsSignal();

    bool deleteSelectedTreesSignal();

    void delActiveNodeSignal();
    bool deleteSelectedNodesSignal();
    void setActiveNodeSignal(int revision, nodeListElement *node, int nodeID);
    void jumpToActiveNodeSignal();
    bool addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    void delSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, segmentListElement *segToDel, int serialize);

    bool newPatchSignal();
    bool setActivePatchSignal(Patch *patch, uint patchID = 0);
    bool deleteSelectedPatchesSignal();
    void jumpToActivePatchSignal();
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();
    void patchSearchChanged();

    void setFocused(KTable *table);
    void keyPressed(QKeyEvent *event);
    void deleteAction();
    void itemsSelected();
    void itemDoubleClicked(QTableWidgetItem*item);

    void displayedNodesChanged(int index);
    void actTreeItemChanged(QTableWidgetItem *item);
    void treeItemChanged(QTableWidgetItem* item);
    void actNodeItemChanged(QTableWidgetItem *item);
    void nodeItemChanged(QTableWidgetItem* item);
    void patchItemChanged(QTableWidgetItem *item);

    // context menu
    void contextMenuCalled(QPoint pos);
    // tree actions
    void setActiveTreeAction();
    void editTreeColor();
    void mergeTreesAction();
    void restoreColorAction();
    void setTreeCommentAction();
    void updateTreeCommentBuffer(QString comment);
    void editTreeComments();
    // node actions
    void setNodeRadiusAction();
    void linkNodesAction();
    void moveNodesAction();
    void splitComponentAction();
    void setNodeCommentAction();
    void updateNodeCommentBuffer(QString comment);
    void editNodeComments();
    void updateNodeRadiusBuffer(double value);
    void editNodeRadii();
    void moveNodesClicked();
    // patch actions
    void addPatchAction();

    // update tree table
    void activeTreeChanged();
    void treeAdded(treeListElement *tree);
    void treesDeleted();
    void treesMerged(int treeID2);
    void treeComponentSplit();

    // update node table
    void activeNodeChanged();
    void nodeAdded();
    void nodesDeleted();
    void branchPushed();
    void branchPopped();
    void nodeCommentChanged(nodeListElement *node);
    void nodeRadiusChanged(nodeListElement *node);
    void nodePositionChanged(nodeListElement *node);
    void updateTreesTable();
    void updateNodesTable();
    void updatePatchesTable();
    void update();
    
    // update patch table
    void activePatchChanged();
    void patchAdded();
    void patchesDeleted();
};

#endif // TREEVIEWTAB_H
