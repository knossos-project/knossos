#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qglobal.h>
#include <qboxlayout.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qformlayout.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlist.h>
#include <qobjectdefs.h>
#include <qpair.h>
#include <qplaintextedit.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qwidget.h>

class TaskManagementWidget;

class TaskUpdateTab : public QWidget {
    Q_OBJECT
    TaskManagementWidget& taskManagementWidget;
    QList<QPair<int, QString>> statusEnum;
    int issueId{-1};
    QString subject;

    QVBoxLayout mainLayout;
    QHBoxLayout headerLayout;
    QFormLayout formLayout;

    QLabel currentTaskLabel;
    QPushButton openInBrowserButton{"Open in browser"};
    QFrame separator;
    QLabel statusLabel;
    QComboBox newStatusCombo;
    QLabel attachAnnotationLabel{"Save and attach annotation"};
    QCheckBox attachAnnotationCheckBox;
    QLabel commentsLabel{"Comments"};
    QPushButton submitButton{"Update task"};

    bool attachmentMismatch();

  public:
    QPlainTextEdit commentsEdit;

    explicit TaskUpdateTab(TaskManagementWidget& taskManagementWidget, QWidget* parent = nullptr);
    void fetchUpdateIssue(int issueId);

  public slots:
    void submit();
    void openInBrowser();

  signals:
    void successfulSubmission();
};
