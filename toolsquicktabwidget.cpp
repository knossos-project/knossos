#include "toolsquicktabwidget.h"

ToolsQuickTabWidget::ToolsQuickTabWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    treeCountLabel = new QLabel("Tree Count: 0");
    nodeCountLabel = new QLabel("Node Count: 0");

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(treeCountLabel, 1, 1);
    gridLayout->addWidget(nodeCountLabel, 1, 2);
    mainLayout->addLayout(gridLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);

    this->activeTreeLabel = new QLabel("Active Tree ID:");
    this->activeTreeSpinBox = new QSpinBox();

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(activeTreeLabel, activeTreeSpinBox);

    mainLayout->addLayout(formLayout);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line2);


    this->setLayout(mainLayout);

}
