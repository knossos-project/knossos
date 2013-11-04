#ifndef TASKMANAGEMENTDETAILSTAB_H
#define TASKMANAGEMENTDETAILSTAB_H

#include <QWidget>
#include <QLabel>

class TaskManagementDetailsTab : public QWidget
{
    Q_OBJECT
    friend class TaskLoginWidget;
    friend class TaskManagementWidget;
public:
    explicit TaskManagementDetailsTab(QWidget *parent = 0);

protected:
    QLabel categoryDescriptionLabel;
    QLabel taskCommentLabel;
    
signals:
    
public slots:
    void setDescription(QString description);
    void setComment(QString comment);
    
};

#endif // TASKMANAGEMENTDETAILSTAB_H
