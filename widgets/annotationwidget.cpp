#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QSplitter>

#include "knossos-global.h"
#include "GuiConstants.h"
#include "annotationwidget.h"

extern stateInfo *state;

AnnotationWidget::AnnotationWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowIcon(QIcon(":/images/icons/graph.png"));
    setWindowTitle("Annotation");
    tabs = new QTabWidget(this);
    treeviewTab = new ToolsTreeviewTab(this);
    commandsTab = new ToolsCommandsTab(this);
    tabs->addTab(treeviewTab, "Tree View");
    tabs->addTab(commandsTab, "Commands");

    treeCountLabel = new QLabel("Total Tree Count: 0");
    nodeCountLabel = new QLabel("Total Node Count: 0");
    nodeCountLabel->setToolTip("Total number of nodes in the skeleton.");
    listedNodesLabel = new QLabel("Currently Listed Nodes: 0");
    listedNodesLabel->setToolTip("Number of nodes currently listed in the table.");
    QHBoxLayout *hLayout = new QHBoxLayout();
    QHBoxLayout *subHLayout = new QHBoxLayout();
    subHLayout->addWidget(listedNodesLabel, 0, Qt::AlignLeft);
    subHLayout->addWidget(nodeCountLabel, 0, Qt::AlignRight);
    hLayout->addWidget(treeCountLabel);
    hLayout->addLayout(subHLayout);


    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    mainLayout->addLayout(hLayout);
    setLayout(mainLayout);

    connect(treeviewTab, &ToolsTreeviewTab::updateAnnotationLabelsSignal, this, &AnnotationWidget::updateLabels);

    connect(commandsTab, SIGNAL(treeActivatedSignal()), treeviewTab, SLOT(treeActivated()));
    connect(commandsTab, SIGNAL(treeAddedSignal(treeListElement*)), treeviewTab, SLOT(treeAdded(treeListElement*)));
    connect(commandsTab, SIGNAL(nodeActivatedSignal()), treeviewTab, SLOT(nodeActivated()));
    connect(commandsTab, SIGNAL(branchPushedSignal()), treeviewTab, SLOT(branchPushed()));
    connect(commandsTab, SIGNAL(branchPoppedSignal()), treeviewTab, SLOT(branchPopped()));

   this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void AnnotationWidget::updateLabels() {
    listedNodesLabel->setText(QString("Currently Listed Nodes: %1").arg(treeviewTab->nodeTable->rowCount()));
    treeCountLabel->setText(QString("Total Tree Count: %1").arg(state->skeletonState->treeElements));
    nodeCountLabel->setText(QString("Total Node Count: %1").arg(state->skeletonState->totalNodeElements));
    commandsTab->update();
}

void AnnotationWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    width = (settings.value(WIDTH).isNull())? 700 : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().bottomRight().x() - width - 20;
        y = QApplication::desktop()->screen()->rect().bottomRight().y() - height - 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();

    if(settings.value(SEARCH_FOR_TREE).isNull() == false) {
        treeviewTab->treeSearchField->setText(settings.value(SEARCH_FOR_TREE).toString());
    }
    else {
        treeviewTab->treeSearchField->setPlaceholderText("search tree");
    }
    if(settings.value(SEARCH_FOR_NODE).isNull() == false) {
        treeviewTab->nodeSearchField->setText(settings.value(SEARCH_FOR_NODE).toString());
    }
    else {
        treeviewTab->nodeSearchField->setPlaceholderText("search node");
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
    if(visible) {
        show();
    }
    else {
        hide();
    }
    setGeometry(x, y, width, height);
    QList<int> list;
    list.append(310);
    list.append(390);
    treeviewTab->splitter->setSizes(list);
}

void AnnotationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(SEARCH_FOR_TREE, treeviewTab->treeSearchField->text());
    settings.setValue(SEARCH_FOR_NODE, treeviewTab->nodeSearchField->text());
    settings.setValue(USE_LAST_RADIUS_AS_DEFAULT, commandsTab->useLastRadiusAsDefaultCheck->isChecked());
    settings.setValue(DEFAULT_NODE_RADIUS, commandsTab->defaultRadiusSpin->value());
    settings.setValue(ENABLE_COMMENT_LOCKING, commandsTab->commentLockingCheck->isChecked());
    settings.setValue(LOCKING_RADIUS, commandsTab->lockingRadiusSpin->value());
    settings.setValue(LOCK_TO_NODES_WITH_COMMENT, commandsTab->commentLockEdit->text());

    settings.endGroup();
}
