#ifndef TOOLSNODESTABWIDGET_H
#define TOOLSNODESTABWIDGET_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QPushButton;
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
