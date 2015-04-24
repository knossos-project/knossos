#include "annotationwidget.h"

#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

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

AnnotationWidget::AnnotationWidget(QWidget *parent) : QDialog(parent) {
    setWindowIcon(QIcon(":/resources/icons/graph.png"));
    setWindowTitle("Annotation");

    tabs.addTab(&treeviewTab, "Tree View");
    tabs.addTab(&skeletonTab, "Skeleton");
    tabs.addTab(&segmentationTab, "Segmentation");
    tabs.addTab(&commandsTab, "Commands");
    tabs.addTab(&commentsTab, "Comments");

    mainLayout.addWidget(&tabs);
    setLayout(&mainLayout);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void AnnotationWidget::setSegmentationVisibility(const bool visible) {
    const auto index = tabs.indexOf(&segmentationTab);
    tabs.setTabEnabled(index, visible);
    const QString tooltip = "Enable the segmentation overlay when loading a dataset";
    tabs.setTabToolTip(index, visible ? "" : tooltip);
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
        treeviewTab.treeSearchField->setText(settings.value(SEARCH_FOR_TREE).toString());
    }
    else {
        treeviewTab.treeSearchField->setPlaceholderText("search tree");
    }
    if(settings.value(SEARCH_FOR_NODE).isNull() == false) {
        treeviewTab.nodeSearchField->setText(settings.value(SEARCH_FOR_NODE).toString());
    }
    else {
        treeviewTab.nodeSearchField->setPlaceholderText("search node");
    }

    if(settings.value(USE_LAST_RADIUS_AS_DEFAULT).isNull() == false) {
        commandsTab.useLastRadiusAsDefaultCheck->setChecked(settings.value(USE_LAST_RADIUS_AS_DEFAULT).toBool());
    }
    else {
        commandsTab.useLastRadiusAsDefaultCheck->setChecked(false);
    }

    if(settings.value(DEFAULT_NODE_RADIUS).isNull() == false) {
        state->skeletonState->defaultNodeRadius = settings.value(DEFAULT_NODE_RADIUS).toDouble();
    }
    else {
        state->skeletonState->defaultNodeRadius = 1.5;
    }
    commandsTab.defaultRadiusSpin->setValue(state->skeletonState->defaultNodeRadius);

    if(settings.value(ENABLE_COMMENT_LOCKING).isNull() == false) {
        commandsTab.commentLockingCheck->setChecked(settings.value(ENABLE_COMMENT_LOCKING).toBool());
    }
    else {
        commandsTab.commentLockingCheck->setChecked(false);
    }

    if(settings.value(LOCKING_RADIUS).isNull() == false) {
        state->skeletonState->lockRadius = settings.value(LOCKING_RADIUS).toInt();
    }
    else {
        state->skeletonState->lockRadius = 10;
    }
    commandsTab.lockingRadiusSpin->setValue(state->skeletonState->lockRadius);

    if(settings.value(LOCK_TO_NODES_WITH_COMMENT).isNull() == false) {
        state->viewerState->lockComment = settings.value(LOCK_TO_NODES_WITH_COMMENT).toString();
    }
    else {
        state->viewerState->lockComment = "seed";
    }
    commandsTab.commentLockEdit->setText(state->viewerState->lockComment);

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
    treeviewTab.splitter->setSizes(list);

    commentsTab.loadSettings();
}

void AnnotationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(SEARCH_FOR_TREE, treeviewTab.treeSearchField->text());
    settings.setValue(SEARCH_FOR_NODE, treeviewTab.nodeSearchField->text());
    settings.setValue(USE_LAST_RADIUS_AS_DEFAULT, commandsTab.useLastRadiusAsDefaultCheck->isChecked());
    settings.setValue(DEFAULT_NODE_RADIUS, commandsTab.defaultRadiusSpin->value());
    settings.setValue(ENABLE_COMMENT_LOCKING, commandsTab.commentLockingCheck->isChecked());
    settings.setValue(LOCKING_RADIUS, commandsTab.lockingRadiusSpin->value());
    settings.setValue(LOCK_TO_NODES_WITH_COMMENT, commandsTab.commentLockEdit->text());
    settings.endGroup();

    commentsTab.saveSettings();
}
