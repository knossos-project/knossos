#include "toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"

ToolsWidget::ToolsWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tools");
    tabs = new QTabWidget(this);

    this->toolsQuickTabWidget = new ToolsQuickTabWidget();
    this->toolsNodesTabWidget = new ToolsNodesTabWidget();
    this->toolsTreesTabWidget = new ToolsTreesTabWidget();

    tabs->addTab(toolsQuickTabWidget, "Quick");
    tabs->addTab(toolsTreesTabWidget, "Trees");
    tabs->addTab(toolsNodesTabWidget, "Nodes");

}

void ToolsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
