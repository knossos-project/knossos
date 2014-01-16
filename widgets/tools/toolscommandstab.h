#ifndef TOOLSCOMMANDSTAB_H
#define TOOLSCOMMANDSTAB_H

#include <QWidget>

#include "knossos-global.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QVBoxLayout;

class ToolsCommandsTab : public QWidget
{
    Q_OBJECT
    friend class AnnotationWidget;

public:
    explicit ToolsCommandsTab(QWidget *parent = 0);
    
protected:
    QLabel *treeLabel;
    QLabel *activeTreeLabel;
    QLabel *activeNodeLabel;
    QLabel *activeTreeIDLabel;
    QLabel *activeNodeIDLabel;
    QSpinBox *activeTreeIDSpin;
    QSpinBox *activeNodeIDSpin;
    QPushButton *jumpToActiveButton;
    QPushButton *newTreeButton;

    QLabel *branchnodesLabel;
    QLabel *branchesOnStackLabel;
    QPushButton *pushBranchButton;
    QPushButton *popBranchButton;

    QLabel *radiusLabel;
    QLabel *defaultRadiusLabel;
    QDoubleSpinBox *defaultRadiusSpin;
    QCheckBox *useLastRadiusAsDefaultCheck;

    QLabel *lockingLabel;
    QCheckBox *commentLockingCheck;
    QLabel *lockingRadiusLabel;
    QSpinBox *lockingRadiusSpin;
    QLabel *lockToNodesLabel;
    QLineEdit *commentLockEdit;
    QPushButton *locktoActiveButton;
    QPushButton *disableCurrentLockButton;

    QVBoxLayout *mainLayout;

signals:
    treeListElement *findTreeByTreeIDSignal(int value);
    nodeListElement *findNodeByNodeIDSignal(int value);
    void setActiveTreeSignal(int id);
    void setActiveNodeSignal(int revision, nodeListElement *node, int id);

    void jumpToNodeSignal();
    treeListElement *addTreeListElement(int sync, int targetRevision, int treeID, color4F color, int serialize);
    void pushBranchNodeSignal(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint,
                              nodeListElement *branchNode, int branchNodeID, int serialize);
    void popBranchNodeSignal();
    void lockPositionSignal(Coordinate coordinate);
    void unlockPositionSignal();

    void updateTreeviewSignal();
public slots:
    void activeTreeIDSpinChanged(int value);
    void activeNodeIDSpinChanged(int value);
    void jumpToActiveButtonClicked();
    void newTreeButtonClicked();
    void pushBranchButtonClicked();
    void popBranchButtonClicked();
    void useLastRadiusAsDefaultCheckChanged(bool on);
    void defaultRadiusSpinChanged(double value);
    void commentLockingCheckChanged(bool on);
    void lockingRadiusSpinChanged(int value);
    void commentLockEditChanged(QString comment);
    void locktoActiveButtonClicked();
    void disableCurrentLockButtonClicked();

    void update();
};

#endif // TOOLSCOMMANDSTAB_H
