#include "toolswidget.h"

ToolsWidget::ToolsWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tools");
    tabs = new QTabWidget(this);
    tabs->addTab(new ToolsQuickTabWidget(), "Quick");
    tabs->addTab(new ToolsTreesTabWidget(), "Trees");
    tabs->addTab(new ToolsNodesTabWidget(), "Nodes");

}

void ToolsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
