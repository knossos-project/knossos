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
class QSplitter;
class QMenu;

struct treeListElement;
struct nodeListElement;

class TreeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit TreeTable(QWidget *parent);

    int droppedOnTreeID;
protected:
    void keyPressEvent(QKeyEvent *event);
    void dropEvent(QDropEvent *event);
signals:
    void updateTreeview();
public slots:
    void deleteTreesAction();
};

class NodeTable : public QTableWidget {
    Q_OBJECT
public:
    explicit NodeTable(QWidget *parent);
protected:
    void keyPressEvent(QKeyEvent *event);
signals:
    void delActiveNodeSignal();
    void updateNodesTable();
public slots:
    void deleteNodesAction();

};

class ToolsTreeviewTab : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreeviewTab(QWidget *parent = 0);

    TreeTable *treeTable;
    NodeTable *nodeTable;

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

    QDialog *treeCommentEditDialog;
    QLineEdit *treeCommentField;
    QPushButton *treeApplyButton;
    QPushButton *treeCancelButton;
    QVBoxLayout *treeCommentLayout;
    QString treeCommentBuffer;

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
    float radiusBuffer;

    // drag'n drop buffers
    int draggedNodeID;
    int displayedNodes;

    void updateTreeColorCells(int row);
    bool matchesSearchString(QString searchString, QString string, bool useRegEx);

    void createTreesContextMenu();
    void createNodesContextMenu();
    void createContextMenuDialogs();
    QPushButton *confirmationPrompt(QString question, QString confirmString);
signals:
    void updateToolsSignal();
    void setActiveNodeSignal(int revision, nodeListElement *node, int nodeID);
    void JumpToActiveNodeSignal();
    bool addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
    void delActiveNodeSignal();
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();

    void displayedNodesChanged(int index);

    void treeItemChanged(QTableWidgetItem* item);
    void treeItemSelected();
    void nodeItemChanged(QTableWidgetItem* item);
    void nodeItemSelected();
    void nodeItemDoubleClicked(QTableWidgetItem* item);
    // tree context menu
    void treeContextMenuCalled(QPoint pos);
    void setActiveTreeAction();
    //bool deleteTreesAction();
    bool mergeTreesAction();
    void restoreColorAction();
    void setTreeCommentAction();
    void updateTreeCommentBuffer(QString comment);
    void editTreeComments();

    // node context menu
    void nodeContextMenuCalled(QPoint pos);
    void setNodeRadiusAction();
    void linkNodesAction();
    void splitComponentAction();
    void setNodeCommentAction();
    void updateNodeCommentBuffer(QString comment);
    void editNodeComments();
    void updateNodeRadiusBuffer(double value);
    void editNodeRadii();

    void updateTreesTable();
    void updateNodesTable();
    void update();
    
};

#endif // TREEVIEWTAB_H
