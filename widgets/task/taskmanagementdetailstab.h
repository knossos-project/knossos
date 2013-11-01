#ifndef TASKMANAGEMENTDETAILSTAB_H
#define TASKMANAGEMENTDETAILSTAB_H

#include <QWidget>
#include <QLabel>

class TaskManagementDetailsTab : public QWidget
{
    Q_OBJECT
public:
    explicit TaskManagementDetailsTab(QWidget *parent = 0);

protected:
    QLabel categoryDescriptionLabel;
    QLabel taskCommentLabel;
    
signals:
    
public slots:
    void updateDescriptionSlot(QString description, QString comment);
    
};

#endif // TASKMANAGEMENTDETAILSTAB_H
