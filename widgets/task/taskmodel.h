#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QJsonObject>
#include <QModelIndex>
#include <QObject>
#include <QPair>
#include <QString>
#include <QtCore>

struct TaskItem {
    int taskIndex;        // -1 means it's the rootItem
    int attachmentsIndex; // -1 means we are referring to a task
    TaskItem* parent;
    QHash<int, TaskItem*> children;

    TaskItem(int taskIndex, int attachmentsIndex, TaskItem* parent);
    ~TaskItem();
};

/**
 * @brief Custom ItemModel to represent a 2 levels tree (attachments below tasks below root)
 */
class TaskModel : public QAbstractItemModel {
    Q_OBJECT
    QJsonObject jsonObject;
    TaskItem* rootItem;
    QString url;
    int issueFetchedCount{0};
    int issueTotalCount{0};
    int taskColumnCount;
    int attachmentColumnCount;

  public:
    bool canFetchMoreValue{false}; // we want to control when we can fetch more data

    explicit TaskModel(QObject* parent = nullptr);
    ~TaskModel();

    // WARN: we consider all QModelIndex non ill-formed
    bool canFetchMore(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void fetchMore(const QModelIndex& parent) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QPair<int, int> issueCounts();
    QPair<bool, QString> fetch(QString url);
    QJsonObject json();
};
