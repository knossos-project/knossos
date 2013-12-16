#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDir>

#include <curl/curl.h>

#include "knossos-global.h"
#include "widgets/mainwindow.h"
#include "skeletonizer.h"
#include "taskmanagementwidget.h"
#include "taskmanagementmaintab.h"

extern stateInfo *state;

TaskManagementWidget::TaskManagementWidget(TaskLoginWidget *loginWidget, QWidget *parent) :
    QDialog(parent)
{
    resize(335, 186);
    mainTab = new TaskManagementMainTab(loginWidget, this);
    detailsTab = new TaskManagementDetailsTab(this);
    tabs = new QTabWidget(this);
    tabs->addTab(mainTab, "General");
    tabs->addTab(detailsTab, "Description");

    QHBoxLayout *layout = new QHBoxLayout(); // add in layout, so that tabs resize, too
    layout->addWidget(tabs);
    setWindowTitle("Task Management");
    setLayout(layout);
    connect(mainTab, SIGNAL(hideSignal()), this, SLOT(hide()));
    connect(mainTab, SIGNAL(setDescriptionSignal(QString)), detailsTab, SLOT(setDescription(QString)));
    connect(mainTab, SIGNAL(setCommentSignal(QString)), detailsTab, SLOT(setComment(QString)));

}

void TaskManagementWidget::closeEvent(QCloseEvent *) {
    this->hide();
}
