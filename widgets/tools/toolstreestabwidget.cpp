#include "toolstreestabwidget.h"

ToolsTreesTabWidget::ToolsTreesTabWidget(QWidget *parent) :
    QWidget(parent)
{
    activeTreeLabel = new QLabel("Active Tree ID:");
    activeTreeSpinBox = new QSpinBox();

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(activeTreeLabel, activeTreeSpinBox);
    mainLayout->addLayout(formLayout);

    deleteActiveTreeButton = new QPushButton("Delete Active Tree");
    newTreeButton = new QPushButton("New Tree (C)");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(deleteActiveTreeButton, 1, 1);
    gridLayout->addWidget(newTreeButton, 1, 2);
    mainLayout->addLayout(gridLayout);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line2);

    setLayout(mainLayout);
}
