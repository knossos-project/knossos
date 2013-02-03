#ifndef TOOLSTREESTABWIDGET_H
#define TOOLSTREESTABWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>

class ToolsTreesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreesTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    QLabel *activeTreeLabel;
    QSpinBox *activeTreeSpinBox;

    QPushButton *deleteActiveTreeButton;
    QPushButton *newTreeButton;

    QLabel *commentLabel;
    QLineEdit *commentField;

    QPushButton *mergeTreesButton, *splitByConnectedComponentsButton, *restoreDefaultColorButton;
    QLabel *id1Label, *id2Label;
    QSpinBox *id1SpinBox, *id2SpinBox;

    QLabel *rLabel, *gLabel, *bLabel, *aLabel;
    QDoubleSpinBox *rSpinBox, *gSpinBox, *bSpinBox, *aSpinBox;

};

#endif // TOOLSTREESTABWIDGET_H
