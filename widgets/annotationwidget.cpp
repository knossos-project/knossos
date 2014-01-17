#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include "knossos-global.h"
#include "GUIConstants.h"
#include "annotationwidget.h"

extern stateInfo *state;

AnnotationWidget::AnnotationWidget(QWidget *parent) :
    QDialog(parent)
{
    tabs = new QTabWidget(this);
    treeviewTab = new ToolsTreeviewTab(this);
    commandsTab = new ToolsCommandsTab(this);
    tabs->addTab(treeviewTab, "Tree View");
    tabs->addTab(commandsTab, "Commands");

    treeCountLabel = new QLabel("Tree Count: 0");
    nodeCountLabel = new QLabel("Node Count: 0");
    nodeCountLabel->setToolTip("Total number of nodes in the skeleton.");
    listedNodesLabel = new QLabel("Listed Nodes: 0");
    listedNodesLabel->setToolTip("Number of nodes currently listed in the table.");
    QHBoxLayout *hLayout = new QHBoxLayout();
    QHBoxLayout *subHLayout = new QHBoxLayout();
    subHLayout->addWidget(listedNodesLabel);
    subHLayout->addWidget(nodeCountLabel);
    hLayout->addWidget(treeCountLabel);
    hLayout->addLayout(subHLayout);


    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    mainLayout->addLayout(hLayout);
    setLayout(mainLayout);

    connect(treeviewTab, SIGNAL(updateToolsSignal()), this, SLOT(update()));
    connect(treeviewTab, SIGNAL(updateListedNodesSignal(int)), this, SLOT(nodesInList(int)));
}

void AnnotationWidget::nodesInList(int n) {
    listedNodesLabel->setText(QString("Listed Nodes: %1").arg(n));
}

void AnnotationWidget::update() {
    treeCountLabel->setText(QString("Tree Count: %1").arg(state->skeletonState->treeElements));
    nodeCountLabel->setText(QString("Node Count: %1").arg(state->skeletonState->totalNodeElements));
    commandsTab->update();
}

void AnnotationWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();

    if(settings.value(SEARCH_FOR_TREE).isNull() == false) {
        treeviewTab->treeCommentField->setText(settings.value(SEARCH_FOR_TREE).toString());
    }
    else {
        treeviewTab->treeCommentField->setPlaceholderText("search tree");
    }
    if(settings.value(SEARCH_FOR_NODE).isNull() == false) {
        treeviewTab->nodeCommentField->setText(settings.value(SEARCH_FOR_NODE).toString());
    }
    else {
        treeviewTab->nodeCommentField->setPlaceholderText("search node");
    }

    if(settings.value(USE_LAST_RADIUS_AS_DEFAULT).isNull() == false) {
        commandsTab->useLastRadiusAsDefaultCheck->setChecked(settings.value(USE_LAST_RADIUS_AS_DEFAULT).toBool());
    }
    else {
        commandsTab->useLastRadiusAsDefaultCheck->setChecked(false);
    }

    if(settings.value(DEFAULT_NODE_RADIUS).isNull() == false) {
        state->skeletonState->defaultNodeRadius = settings.value(DEFAULT_NODE_RADIUS).toDouble();
    }
    else {
        state->skeletonState->defaultNodeRadius = 1.5;
    }
    commandsTab->defaultRadiusSpin->setValue(state->skeletonState->defaultNodeRadius);

    if(settings.value(ENABLE_COMMENT_LOCKING).isNull() == false) {
        commandsTab->commentLockingCheck->setChecked(settings.value(ENABLE_COMMENT_LOCKING).toBool());
    }
    else {
        commandsTab->commentLockingCheck->setChecked(false);
    }

    if(settings.value(LOCKING_RADIUS).isNull() == false) {
        state->skeletonState->lockRadius = settings.value(LOCKING_RADIUS).toInt();
    }
    else {
        state->skeletonState->lockRadius = 10;
    }
    commandsTab->lockingRadiusSpin->setValue(state->skeletonState->lockRadius);

    if(settings.value(LOCK_TO_NODES_WITH_COMMENT).isNull() == false) {
        state->viewerState->gui->lockComment = settings.value(LOCK_TO_NODES_WITH_COMMENT).toString();
    }
    else {
        state->viewerState->gui->lockComment = "seed";
    }
    commandsTab->commentLockEdit->setText(QString(state->viewerState->gui->lockComment));

    settings.endGroup();
    setGeometry(x, y, width, height);
}

void AnnotationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(SEARCH_FOR_TREE, treeviewTab->treeCommentField->text());
    settings.setValue(SEARCH_FOR_NODE, treeviewTab->nodeCommentField->text());
    settings.setValue(USE_LAST_RADIUS_AS_DEFAULT, commandsTab->useLastRadiusAsDefaultCheck->isChecked());
    settings.setValue(DEFAULT_NODE_RADIUS, commandsTab->defaultRadiusSpin->value());
    settings.setValue(ENABLE_COMMENT_LOCKING, commandsTab->commentLockingCheck->isChecked());
    settings.setValue(LOCKING_RADIUS, commandsTab->lockingRadiusSpin->value());
    settings.setValue(LOCK_TO_NODES_WITH_COMMENT, commandsTab->commentLockEdit->text());

    settings.endGroup();
}
