#pragma once

#include "widgets/task/taskmodel.h"

#include <QBoxLayout>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <QtCore/QObject>
#include <QtCore/QtGlobal>

class TaskManagementWidget;
class QModelIndex;
class QPoint;

// each list maps a comboBox index to the corresponding id and name from Redmine
struct Enumerations {
    QList<QPair<int, QString>> statusEnum;
    QList<QPair<int, QString>> priorityEnum;
};

class ReadOnlyStandardItemModel : public QStandardItemModel {
  public:
    Qt::ItemFlags flags(const QModelIndex& index) const override;
};

/**
 * @brief Keeps parents who have 1+ child that fits the filter
 */
class TreeFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

  protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override;
};

class TaskTreeView : public QTreeView {
    Q_OBJECT

  public:
    using QTreeView::QTreeView;
    int width{800};

  protected:
    QSize sizeHint() const override;
};

class TaskSelectionTab : public QWidget {
    Q_OBJECT

    TaskManagementWidget& taskManagementWidget;
    Enumerations enumerations;
    int defaultColumnCount{10}; // first k columns are default

    QVBoxLayout mainLayout;
    QHBoxLayout searchLayout;

    QVBoxLayout leftPartSearchLayout;
    QLineEdit projectSearchBar;
    QStandardItemModel projectItemModel;
    TreeFilterProxyModel projectProxyModel;
    QTreeView projectTreeView;

    QVBoxLayout rightPartSearchLayout;
    QVBoxLayout statusChoiceLayout;
    QLabel statusChoiceLabel{"Status"};
    QComboBox statusChoiceCombo;
    QVBoxLayout priorityLayout;
    QLabel priorityLabel{"Priority"};
    QComboBox priorityCombo;
    QHBoxLayout assignedToMeLayout;
    QCheckBox assignedToMeCheckBox;
    QLabel assignedToMeLabel{"Only those assigned to me"};
    QPushButton fetchButton{"Fetch"};
    QFrame separator;

    QHBoxLayout taskCountFetchMoreLayout;
    QLabel taskCountLabel;
    QPushButton fetchMoreButton{"Fetch more"};

    TaskModel taskModel;
    QSortFilterProxyModel taskProxyModel;
    TaskTreeView taskTreeView;

    void adjustTaskTreeView();
    QList<QString> enumNames(QList<QPair<int, QString>> enumeration);
    bool fetchEnumerations();
    bool fetchProjects();
    QString issuesUrlParams();
    bool saveAndLoadFile(const QString& filename, const QByteArray& content);
    void showDefaultColumns();

  public:
    explicit TaskSelectionTab(TaskManagementWidget& taskManagementWidget,
                              QWidget* parent = nullptr);
    bool resetSearch();
    bool updateAndRefreshWidget();

  public slots:
    void fetchOpenAttachment(TaskItem* taskItem);
    void onFetchMoreButtonCLick();
    void onTaskTreeContextMenu(const QPoint& point);
    void onTaskTreeDoubleCLick(const QModelIndex& index);
    void onTaskTreeHeaderContextMenu(const QPoint& point);

  signals:
    void readyOpenTask(int issueID);
};
