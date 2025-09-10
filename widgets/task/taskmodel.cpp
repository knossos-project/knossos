#include "taskmodel.h"

#include "network.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QList>
#include <QLocale>
#include <QUrl>
#include <QVariant>

#include <algorithm>

TaskItem::TaskItem(int taskIndex, int attachmentsIndex, TaskItem* parent) :
    taskIndex{taskIndex},
    attachmentsIndex{attachmentsIndex},
    parent{parent} {}

TaskItem::~TaskItem() {
    for (auto taskItem : children.values()) {
        delete taskItem;
    }
    children.clear();
}

TaskModel::TaskModel(QObject* parent) :
    QAbstractItemModel(parent),
    rootItem{new TaskItem(-1, -1, nullptr)},
    taskColumnCount{20},
    attachmentColumnCount{1} {}

TaskModel::~TaskModel() { delete rootItem; }

bool TaskModel::canFetchMore(const QModelIndex& parent) const {
    return canFetchMoreValue && parent == QModelIndex() && issueTotalCount &&
           issueFetchedCount != issueTotalCount;
}

int TaskModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? attachmentColumnCount : taskColumnCount;
}

QVariant TaskModel::data(const QModelIndex& index, int role) const {
    const QList<int> allowedRoles = {Qt::DisplayRole, Qt::ToolTipRole, Qt::UserRole};
    if (!index.isValid() || !allowedRoles.contains(role))
        return QVariant();

    auto taskItem = static_cast<TaskItem*>(index.internalPointer());
    QVariant data; // QString for Display/ToolTip roles, sortable types for UserRole
    QLocale locale;
    auto nthIssue = jsonObject["issues"].toArray()[taskItem->taskIndex].toObject();
    if (taskItem->attachmentsIndex == -1) {
        switch (index.column()) {
        case 0:
            data = nthIssue["project"].toObject()["name"].toString();
            break;
        case 1:
            data = nthIssue["status"].toObject()["name"].toString();
            break;
        case 2:
            data = nthIssue["priority"].toObject()["name"].toString();
            break;
        case 3:
            data = nthIssue["subject"].toString();
            break;
        case 4:
            data = nthIssue["description"].toString();
            break;
        case 5:
            data = nthIssue["author"].toObject()["name"].toString();
            break;
        case 6:
            data = nthIssue["assigned_to"].toObject()["name"].toString();
            break;
        case 7: {
            auto date = QDate::fromString(nthIssue["start_date"].toString(), Qt::ISODate);
            if (role == Qt::UserRole)
                data = date;
            else
                data = locale.toString(date, locale.dateFormat(QLocale::ShortFormat));
            break;
        }
        case 8: {
            auto date = QDate::fromString(nthIssue["due_date"].toString(), Qt::ISODate);
            if (role == Qt::UserRole)
                data = date;
            else
                data = locale.toString(date, locale.dateFormat(QLocale::ShortFormat));
            break;
        }
        case 9: {
            auto dateTime = QDateTime::fromString(nthIssue["updated_on"].toString(), Qt::ISODate);
            if (role == Qt::UserRole)
                data = dateTime;
            else
                data = locale.toString(dateTime, locale.dateTimeFormat(QLocale::ShortFormat));
            break;
        }
        case 10: {
            auto dateTime = QDateTime::fromString(nthIssue["created_on"].toString(), Qt::ISODate);
            if (role == Qt::UserRole)
                data = dateTime;
            else
                data = locale.toString(dateTime, locale.dateTimeFormat(QLocale::ShortFormat));
            break;
        }
        case 11: {
            auto dateTime = QDateTime::fromString(nthIssue["closed_on"].toString(), Qt::ISODate);
            if (role == Qt::UserRole)
                data = dateTime;
            else
                data = locale.toString(dateTime, locale.dateTimeFormat(QLocale::ShortFormat));
            break;
        }
        case 12:
            data = nthIssue["id"].toInt();
            break;
        case 13:
            data = nthIssue["tracker"].toObject()["name"].toString();
            break;
        case 14:
            data = nthIssue["done_ratio"].toInt();
            break;
        case 15:
            data = nthIssue["is_private"].toBool();
            break;
        case 16:
            data = nthIssue["estimated_hours"].toString().toFloat();
            break;
        case 17:
            data = nthIssue["total_estimated_hours"].toString().toFloat();
            break;
        case 18:
            data = nthIssue["spent_hours"].toString().toFloat();
            break;
        case 19:
            data = nthIssue["total_spent_hours"].toString().toFloat();
            break;
        default: // unreachable
            return QVariant();
        };
    } else {
        data = nthIssue["attachments"]
                   .toArray()[taskItem->attachmentsIndex]
                   .toObject()["filename"]
                   .toString();
    }

    if (role == Qt::DisplayRole) {
        auto string = data.value<QString>();
        auto newLineIndex = string.indexOf('\n');
        data = string.left(newLineIndex == -1 ? string.length() : newLineIndex);
    }
    return data;
}

void TaskModel::fetchMore(const QModelIndex& parent) {
    if (parent != QModelIndex())
        return;

    auto res = Network::singleton().refresh(url + "&offset=" + QString::number(issueFetchedCount));
    if (!res.first)
        return;
    auto newJsonObject = QJsonDocument::fromJson(res.second).object();
    auto newCount = std::min(newJsonObject["limit"].toInt(),
                             newJsonObject["total_count"].toInt() - issueFetchedCount);

    beginInsertRows(QModelIndex(), issueFetchedCount, issueFetchedCount + newCount - 1);
    // merging jsons objects
    auto issuesArray = jsonObject["issues"].toArray();
    for (auto jsonvalue : newJsonObject["issues"].toArray()) {
        issuesArray.append(jsonvalue);
    }
    jsonObject["issues"] = issuesArray;
    issueFetchedCount += newCount;
    endInsertRows();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation == Qt::Vertical)
        return QVariant();
    switch (section) {
    case 0:
        return "Project";
    case 1:
        return "Status";
    case 2:
        return "Priority";
    case 3:
        return "Subject";
    case 4:
        return "Description";
    case 5:
        return "Author";
    case 6:
        return "Assigned to";
    case 7:
        return "Start date";
    case 8:
        return "Due date";
    case 9:
        return "Updated on";
    case 10:
        return "Created on";
    case 11:
        return "Closed on";
    case 12:
        return "Id";
    case 13:
        return "Tracker";
    case 14:
        return "Done ratio";
    case 15:
        return "Is private";
    case 16:
        return "Est. hours";
    case 17:
        return "Tot est. hours";
    case 18:
        return "Spent hours";
    case 19:
        return "Tot. spent hours";
    default:
        return QVariant();
    }
}

QModelIndex TaskModel::index(int row, int column, const QModelIndex& parent) const {
    auto parentTaskItem =
        parent.isValid() ? static_cast<TaskItem*>(parent.internalPointer()) : rootItem;

    if (columnCount(parent) <= column || rowCount(parent) <= row) {
        return QModelIndex();
    }

    TaskItem* taskItem;
    if (parentTaskItem->children.contains(row)) {
        taskItem = parentTaskItem->children[row];
    } else {
        taskItem = parent.isValid() ? new TaskItem(parent.row(), row, parentTaskItem)
                                    : new TaskItem(row, -1, parentTaskItem);
        parentTaskItem->children[row] = taskItem;
    }
    return createIndex(row, column, taskItem);
}

QModelIndex TaskModel::parent(const QModelIndex& index) const {
    auto taskItem = static_cast<TaskItem*>(index.internalPointer());
    if (taskItem->attachmentsIndex == -1)
        return QModelIndex();
    // only items in the first column have children so we set the column to 0
    return createIndex(taskItem->taskIndex, 0, taskItem->parent);
}

int TaskModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) // parent is the root
        return issueFetchedCount;
    if (parent.parent().isValid()) // parent is an attachment
        return 0;
    // parent is a task
    return jsonObject["issues"].toArray()[parent.row()].toObject()["attachments"].toArray().size();
}

QPair<int, int> TaskModel::issueCounts() {
    return QPair<int, int>(issueFetchedCount, issueTotalCount);
}

QJsonObject TaskModel::json() { return jsonObject; }

QPair<bool, QString> TaskModel::fetch(QString url) {
    this->url = url;
    auto res = Network::singleton().refresh(url);
    if (!res.first)
        return res;
    jsonObject = QJsonDocument::fromJson(res.second).object();

    beginResetModel();
    delete rootItem;
    rootItem = new TaskItem(-1, -1, nullptr);
    issueTotalCount = jsonObject["total_count"].toInt();
    issueFetchedCount = std::min(jsonObject["limit"].toInt(), jsonObject["total_count"].toInt());
    endResetModel();

    return QPair(true, "");
}
