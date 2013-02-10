#include "toolsquicktabwidget.h"

extern struct stateInfo *state;

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


    activeNodeLabel = new QLabel("Active Node ID:");
    activeNodeSpinBox = new QSpinBox();
    activeNodeSpinBox->setDisabled(true);

    QFormLayout *formLayout2 = new QFormLayout();

    formLayout2->addRow(activeNodeLabel, activeNodeSpinBox);
    mainLayout->addLayout(formLayout2);

    xLabel = new QLabel("x: 0");
    yLabel = new QLabel("y: 0");
    zLabel = new QLabel("z: 0");


    QGridLayout *gridLayout2 = new QGridLayout();
    gridLayout2->addWidget(xLabel, 1, 1);
    gridLayout2->addWidget(yLabel, 1, 2);
    gridLayout2->addWidget(zLabel, 1, 3);
    mainLayout->addLayout(gridLayout2);


    commentLabel = new QLabel("Comment:");
    commentField = new QLineEdit();

    searchForLabel = new QLabel("Search For:");
    searchForField = new QLineEdit();

    QFormLayout *formLayout3 = new QFormLayout();
    formLayout3->addRow(commentLabel, commentField);
    formLayout3->addRow(searchForLabel, searchForField);
    mainLayout->addLayout(formLayout3);

    findNextButton = new QPushButton("Find (n)ext");
    findPreviousButton = new QPushButton("Find (p)revious");

    QGridLayout *gridLayout3 = new QGridLayout();
    gridLayout3->addWidget(findNextButton, 1, 1);
    gridLayout3->addWidget(findPreviousButton, 1, 2);
    mainLayout->addLayout(gridLayout3);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line3);

    branchPointLabel = new QLabel("Branchpoints");
    mainLayout->addWidget(branchPointLabel);

    onStackLabel = new QLabel("On Stack: 0");
    mainLayout->addWidget(onStackLabel);

    pushBranchNodeButton = new QPushButton("Push (B)ranch Node");
    popBranchNodeButton = new QPushButton("Pop & (J)ump ");

    QGridLayout *gridLayout4 = new QGridLayout();
    gridLayout4->addWidget(pushBranchNodeButton, 1, 1);
    gridLayout4->addWidget(popBranchNodeButton, 1, 2);

    mainLayout->addLayout(gridLayout4);

    this->setLayout(mainLayout);


}

void ToolsQuickTabWidget::activeTreeIdSlot(int value) {
    state->viewerState->gui->activeTreeID = value;
}
