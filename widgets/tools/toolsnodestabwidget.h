#ifndef TOOLSNODESTABWIDGET_H
#define TOOLSNODESTABWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QFrame>
#include <QPushButton>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>

class ToolsNodesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsNodesTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    
protected:
    QLabel *activeNodeIdLabel, *idLabel;
    QSpinBox *activeNodeIdSpinBox, *idSpinBox;
    QPushButton *jumpToNodeButton, *deleteNodeButton, *linkNodeWithButton;
};

#endif // TOOLSNODESTABWIDGET_H
