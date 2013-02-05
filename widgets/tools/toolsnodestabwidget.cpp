#include "toolsnodestabwidget.h"

ToolsNodesTabWidget::ToolsNodesTabWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    activeNodeIdLabel = new QLabel("Active Node ID");
    activeNodeIdSpinBox = new QSpinBox();

    jumpToNodeButton = new QPushButton("Jump to node(s)");
    deleteNodeButton = new QPushButton("Delete Node(Del)");
    linkNodeWithButton = new QPushButton("Link Node with(Shift + Click)");

    idLabel = new QLabel("ID:");
    idSpinBox = new QSpinBox();

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(activeNodeIdLabel, activeNodeIdSpinBox);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(jumpToNodeButton, 0, 0);
    gridLayout->addWidget(deleteNodeButton, 0, 1);
    gridLayout->addWidget(linkNodeWithButton, 1, 0);
    gridLayout->addWidget(idLabel, 1, 1);
    gridLayout->addWidget(idSpinBox, 1, 2);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
}
