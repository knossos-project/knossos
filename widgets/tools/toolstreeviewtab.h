#ifndef TREEVIEWTAB_H
#define TREEVIEWTAB_H

#include "widgets/tools/nodetable.h"
#include "widgets/tools/treetable.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QTableWidget>
#include <QWidget>

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
    QRadioButton selectedNodesRadio;//i wanna be special

    QCheckBox *branchNodesChckBx;
    QCheckBox *commentNodesChckBx;
    QLabel *displayedNodesLabel;
    QComboBox *displayedNodesCombo;
    QMenu *treeContextMenu;
    QMenu *nodeContextMenu;

    QSplitter *splitter;
    QWidget *treeSide;
    QWidget *nodeSide;

    QLabel treeCountLabel;
    QLabel nodeCountLabel;
    QHBoxLayout bottomHLayout;

    QVBoxLayout *mainLayout;

    //edit dialogs values
    QString nodeCommentBuffer;
    float radiusBuffer;
    QString treeCommentBuffer;

    // drag'n drop buffers
    int draggedNodeID;
    int displayedNodes;

    bool askExtractConnectedComponent = true;

    void updateTreeColorCell(TreeTable *table, int row);
    bool matchesSearchString(QString searchString, QString string, bool useRegEx);

    QPushButton *confirmationPrompt(QString question, QString confirmString);

protected:
    void insertTree(treeListElement *tree, TreeTable *table);
    void insertNode(const nodeListElement * node, NodeTable * table);
    void setText(TreeTable *table, QTableWidgetItem *item, QString text);
    void setText(NodeTable *table, QTableWidgetItem *item, QString text);
    int getActiveTreeRow();
    int getActiveNodeRow();
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();

    void displayedNodesChanged(int index);
    void actTreeItemChanged(QTableWidgetItem *item);
    void activeTreeTableSelectionChanged();
    void treeItemChanged(QTableWidgetItem* item);
    void treeTableSelectionChanged();
    void treeItemDoubleClicked(QTableWidgetItem* item);

    void actNodeItemChanged(QTableWidgetItem *item);
    void nodeItemChanged(QTableWidgetItem* item);
    void activeNodeTableSelectionChanged();
    void nodeTableSelectionChanged();
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
    void extractConnectedComponentAction();
    void setNodeCommentAction();
    void deleteNodesAction();

    // update tree table
    void recreateTreesTable();
    void applyTreeSelection();
    void treeActivated();
    void treeAddedOrChanged();
    void treesMerged(int treeID1, int treeID2);

    // update node table
    void recreateNodesTable();
    void applyNodeSelection();
    void nodeActivated();
    void nodeAdded(const nodeListElement &node);
    void branchPushed();
    void branchPopped();
    void nodeEdited(const nodeListElement &node);

    void updateLabels();
    void resetData();

    void showSelectedTrees();
    void hideSelectedTrees();
};

#endif // TREEVIEWTAB_H
