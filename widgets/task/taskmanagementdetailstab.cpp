#include <QVBoxLayout>
#include <QFormLayout>

#include "knossos-global.h"
#include "taskmanagementdetailstab.h"

extern struct stateInfo *state;
TaskManagementDetailsTab::TaskManagementDetailsTab(QWidget *parent) :
    QWidget(parent)
{
    categoryDescriptionLabel.setText("Category description: ");
    categoryDescriptionLabel.setWordWrap(true);
    taskCommentLabel.setText("Task comment: ");
    taskCommentLabel.setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(&categoryDescriptionLabel);
    layout->addWidget(&taskCommentLabel);

    setLayout(layout);
}

void TaskManagementDetailsTab::updateDescriptionSlot(QString description, QString comment) {
    categoryDescriptionLabel.setText(QString("<b>Category %1:</b> %2").arg(taskState::getCategory()).arg(description));
    taskCommentLabel.setText(QString("<b>Task %1:</b> %2").arg(taskState::getTask()).arg(comment));
}
