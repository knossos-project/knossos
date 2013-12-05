#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QDialog>
#include <QTableWidget>


class QTableWidgetItem;
class QHBoxLayout;
class QVBoxLayout;
class QLineEdit;
class QLabel;
class QRadioButton;
class QCheckBox;
class QPushButton;
class QDoubleSpinBox;
class QMenu;

class TreeTable;
struct treeListElement;
struct nodeListElement;

class Treeview : public QDialog
{
    Q_OBJECT
public:
    explicit Treeview(QWidget *parent = 0);
    
protected:
    TreeTable *treeTable;
    QTableWidget *nodeTable;

    QLineEdit *treeSearchField;
    QLineEdit *nodeSearchField;
    QCheckBox *treeRegExCheck;
    QCheckBox *nodeRegExCheck;
    QRadioButton *nodesOfSelectedTreesRadio;
    QRadioButton *allNodesRadio;
    QCheckBox *branchNodesChckBx;
    QCheckBox *commentNodesChckBx;

    QMenu *treeContextMenu;
    QMenu *nodeContextMenu;

    QHBoxLayout *mainLayout;

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
    void delActiveNodeSignal();
public slots:
    void treeSearchChanged();
    void nodeSearchChanged();

    void treeItemChanged(QTableWidgetItem* item);
    void treeItemSelected(int row, int col);
    void treeItemDoubleClicked(QTableWidgetItem* item);
    void nodeItemChanged(QTableWidgetItem* item);
    void nodeItemSelected(int row, int col);
    void nodeItemDoubleClicked(QTableWidgetItem* item);
    // tree context menu
    void treeContextMenuCalled(QPoint pos);
    void setActiveTreeAction();
    bool deleteTreesAction();
    bool mergeTreesAction();
    void restoreColorAction();
    void setTreeCommentAction();
    void updateTreeCommentBuffer(QString comment);
    void editTreeComments();

    // node context menu
    void nodeContextMenuCalled(QPoint pos);
    void setNodeRadiusAction();
    bool splitComponentAction();
    void deleteNodesAction();
    void setNodeCommentAction();
    void updateNodeCommentBuffer(QString comment);
    void editNodeComments();
    void updateNodeRadiusBuffer(double value);
    void editNodeRadii();

    void updateTreesTable();
    void updateNodesTable();
    void update();
    
};

#endif // TREEVIEW_H
