#include "toolstreestabwidget.h"

ToolsTreesTabWidget::ToolsTreesTabWidget(QWidget *parent) :
    QWidget(parent)
{
    activeTreeLabel = new QLabel("Active Tree ID:");
    activeTreeSpinBox = new QSpinBox();

    QVBoxLayout *mainLayout = new QVBoxLayout();

    deleteActiveTreeButton = new QPushButton("Delete Active Tree");
    newTreeButton = new QPushButton("New Tree (C)");

    commentLabel = new QLabel("Comment");
    commentField = new QLineEdit();

    mergeTreesButton = new QPushButton("Merge Trees");
    id1Label = new QLabel("ID 1:");
    id1SpinBox = new QSpinBox();
    id2Label = new QLabel("ID 2:");
    id2SpinBox = new QSpinBox();

    splitByConnectedComponentsButton = new QPushButton("Split By Connected Components");
    rLabel = new QLabel("R:");
    gLabel = new QLabel("G:");
    bLabel = new QLabel("B:");
    aLabel = new QLabel("A:");

    rSpinBox = new QDoubleSpinBox();
    gSpinBox = new QDoubleSpinBox();
    bSpinBox = new QDoubleSpinBox();
    aSpinBox = new QDoubleSpinBox();

    mergeTreesButton = new QPushButton("Merge Trees");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    QFrame *line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);

    QFrame *line5 = new QFrame();
    line5->setFrameShape(QFrame::HLine);
    line5->setFrameShadow(QFrame::Sunken);

    QFrame *line6 = new QFrame();
    line6->setFrameShape(QFrame::HLine);
    line6->setFrameShadow(QFrame::Sunken);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(activeTreeLabel, 0, 0);
    gridLayout->addWidget(activeTreeSpinBox, 0, 1);
    gridLayout->addWidget(line, 1, 0);
    gridLayout->addWidget(line2, 1, 1);
    gridLayout->addWidget(deleteActiveTreeButton, 2, 0);
    gridLayout->addWidget(newTreeButton, 2, 1);
    gridLayout->addWidget(line3, 3, 0);
    gridLayout->addWidget(line4, 3, 1);
    gridLayout->addWidget(commentLabel, 4, 0);
    gridLayout->addWidget(commentField, 4, 1);
    gridLayout->addWidget(line5, 5, 0);
    gridLayout->addWidget(line6, 5, 1);
    gridLayout->addWidget(mergeTreesButton, 6, 0);
    gridLayout->addWidget(id1Label, 6, 1);
    gridLayout->addWidget(id1SpinBox, 6, 2);

    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
}
