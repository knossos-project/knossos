#ifndef TOOLSQUICKTABWIDGET_H
#define TOOLSQUICKTABWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include "knossos-global.h"

class ToolsQuickTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsQuickTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    void activeTreeIdSlot(int value);
public:
    QLabel *treeCountLabel, *nodeCountLabel;
    QLabel *activeTreeLabel, *activeNodeLabel;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *activeTreeSpinBox, *activeNodeSpinBox;

    QLabel *commentLabel, *searchForLabel;
    QLineEdit *commentField, *searchForField;
    QPushButton *findNextButton, *findPreviousButton;

    QLabel *branchPointLabel, *onStackLabel;
    QPushButton *pushBranchNodeButton, *popBranchNodeButton;
};

#endif // TOOLSQUICKTABWIDGET_H
