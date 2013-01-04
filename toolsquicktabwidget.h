#ifndef TOOLSQUICKTABWIDGET_H
#define TOOLSQUICKTABWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFrame>
#include <QSpinBox>

class ToolsQuickTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsQuickTabWidget(QWidget *parent = 0);
    
signals:
    
public slots:
protected:
    QLabel *treeCountLabel, *nodeCountLabel;
    QLabel *activeTreeLabel, *activeNodeLabel;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *activeTreeSpinBox, *activeNodeSpinBox;

};

#endif // TOOLSQUICKTABWIDGET_H
