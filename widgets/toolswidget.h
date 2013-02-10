#ifndef TOOLSWIDGET_H
#define TOOLSWIDGET_H

#include <QDialog>
#include <QTabWidget>

class ToolsQuickTabWidget;
class ToolsTreesTabWidget;
class ToolsNodesTabWidget;
class ToolsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ToolsWidget(QWidget *parent = 0);
    ToolsQuickTabWidget *toolsQuickTabWidget;
    ToolsNodesTabWidget *toolsNodesTabWidget;
    ToolsTreesTabWidget *toolsTreesTabWidget;
signals:
    
public slots:
protected:
    void closeEvent(QCloseEvent *event);
    QTabWidget *tabs;

};

#endif // TOOLSWIDGET_H
