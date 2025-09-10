#include "taskselectiontab.h"

#include "annotation/annotation.h"
#include "network.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/mainwindow.h"
#include "widgets/task/taskmanagementwidget.h"
#include "widgets/task/taskmodel.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QHashIterator>
#include <QHeaderView>
#include <QIODevice>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QRect>
#include <QScreen>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTreeView>
#include <QUrl>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <initializer_list>

class QPoint;

Qt::ItemFlags ReadOnlyStandardItemModel::flags(const QModelIndex& index) const {
    (void)index;
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

bool TreeFilterProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const {
    auto index = sourceModel()->index(row, filterKeyColumn(), parent);

    if (sourceModel()->data(index).toString().contains(filterRegExp())) {
        return true;
    }

    auto rowCount = sourceModel()->rowCount(index);
    for (int i = 0; i < rowCount; ++i) {
        if (filterAcceptsRow(i, index)) {
            return true;
        }
    }
    return false;
}

QSize TaskTreeView::sizeHint() const { return QSize(width, 300); }

TaskSelectionTab::TaskSelectionTab(TaskManagementWidget& taskManagementWidget, QWidget* parent) :
    QWidget(parent),
    taskManagementWidget{taskManagementWidget} {
    setWindowIcon(QIcon(":/resources/icons/tasks-management.png"));
    setWindowTitle("Task Management");

    for (auto&& label : {&statusChoiceLabel, &priorityLabel, &assignedToMeLabel, &taskCountLabel}) {
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }
    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    projectSearchBar.setPlaceholderText("Search a project");
    assignedToMeCheckBox.setChecked(true);
    fetchButton.setStyleSheet("background-color: #90EE90; font-weight: bold;");
    fetchMoreButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    projectProxyModel.setSourceModel(&projectItemModel);
    projectProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    projectProxyModel.setFilterKeyColumn(0);
    projectTreeView.setModel(&projectProxyModel);
    projectTreeView.setHeaderHidden(true);
    projectTreeView.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    taskProxyModel.setSourceModel(&taskModel);
    taskProxyModel.setSortRole(Qt::UserRole);
    taskTreeView.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    taskTreeView.setModel(&taskProxyModel);
    taskTreeView.setSortingEnabled(true);
    taskTreeView.setContextMenuPolicy(Qt::CustomContextMenu);
    taskTreeView.header()->setContextMenuPolicy(Qt::CustomContextMenu);
    showDefaultColumns();

    mainLayout.addLayout(&searchLayout);
    mainLayout.addWidget(&separator);
    mainLayout.addWidget(&taskTreeView);
    mainLayout.addLayout(&taskCountFetchMoreLayout);

    searchLayout.addLayout(&leftPartSearchLayout);
    searchLayout.addLayout(&rightPartSearchLayout);

    leftPartSearchLayout.addWidget(&projectSearchBar);
    leftPartSearchLayout.addWidget(&projectTreeView);

    rightPartSearchLayout.addLayout(&statusChoiceLayout);
    rightPartSearchLayout.addLayout(&priorityLayout);
    rightPartSearchLayout.addLayout(&assignedToMeLayout);
    rightPartSearchLayout.addWidget(&fetchButton);

    statusChoiceLayout.addWidget(&statusChoiceLabel);
    statusChoiceLayout.addWidget(&statusChoiceCombo);

    priorityLayout.addWidget(&priorityLabel);
    priorityLayout.addWidget(&priorityCombo);

    assignedToMeLayout.addWidget(&assignedToMeCheckBox);
    assignedToMeLayout.addWidget(&assignedToMeLabel);
    assignedToMeLayout.setAlignment(Qt::AlignLeft);

    taskCountFetchMoreLayout.addWidget(&taskCountLabel);
    taskCountFetchMoreLayout.addWidget(&fetchMoreButton);
    taskCountFetchMoreLayout.setAlignment(Qt::AlignLeft);

    setLayout(&mainLayout);

    QObject::connect(&taskTreeView, &TaskTreeView::customContextMenuRequested, this,
                     &TaskSelectionTab::onTaskTreeContextMenu);
    QObject::connect(&taskTreeView, &QTreeView::doubleClicked, this,
                     &TaskSelectionTab::onTaskTreeDoubleCLick);
    QObject::connect(taskTreeView.header(), &TaskTreeView::customContextMenuRequested, this,
                     &TaskSelectionTab::onTaskTreeHeaderContextMenu);
    QObject::connect(&fetchButton, &QPushButton::clicked, this,
                     &TaskSelectionTab::updateAndRefreshWidget);
    QObject::connect(&projectSearchBar, &QLineEdit::textChanged, [this](QString search) {
        projectProxyModel.setFilterWildcard("*" + search + "*");
        if (!search.isEmpty()) {
            projectTreeView.expandAll();
        }
    });
    QObject::connect(&fetchMoreButton, &QPushButton::clicked, this,
                     &TaskSelectionTab::onFetchMoreButtonCLick);
}

bool loginGuardMsg() {
    QMessageBox overwriteMsg(QApplication::activeWindow());
    overwriteMsg.setIcon(QMessageBox::Question);
    overwriteMsg.setText(
        QObject::tr("You have unsaved changes that will be lost on loading the annotation."));
    const auto* discard = overwriteMsg.addButton(
        QObject::tr("Discard current changes load the annotation."), QMessageBox::AcceptRole);
    overwriteMsg.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
    overwriteMsg.exec();
    return overwriteMsg.clickedButton() == discard;
}

void TaskSelectionTab::adjustTaskTreeView() {
    // TODO: not great, havn't found better
    for (int i = 0; i <= taskModel.columnCount(); i++) {
        taskTreeView.resizeColumnToContents(i);
        taskTreeView.setColumnWidth(i, std::min(taskTreeView.columnWidth(i), 200) + 5);
    }

    auto width =
        taskTreeView.verticalScrollBar()->sizeHint().width() + taskTreeView.frameWidth() * 2;
    for (auto i = 0; i < taskModel.columnCount(); i++) {
        width += taskTreeView.columnWidth(i);
    }
    taskTreeView.width = std::min(width, screen()->geometry().width() * 2 / 3);
    for (int i = 0; i < taskModel.rowCount(QModelIndex()); ++i) {
        QModelIndex idTask = taskModel.index(i, 0, QModelIndex());
        for (int j = 0; j < taskModel.rowCount(idTask); ++j) {
            taskTreeView.setFirstColumnSpanned(j, taskProxyModel.mapFromSource(idTask), true);
        }
    }
    updateGeometry();
}

QList<QString> TaskSelectionTab::enumNames(QList<QPair<int, QString>> enumeration) {
    QList<QString> res;
    for (const auto& pair : enumeration) {
        res.append(pair.second);
    }
    return res;
}

bool TaskSelectionTab::fetchEnumerations() {
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;

    const auto statusRes =
        Network::singleton().refresh(taskManagementWidget.baseUrl + "/issue_statuses.json?key=" +
                                     taskManagementWidget.loadApiKey());
    const auto priorityRes = Network::singleton().refresh(
        taskManagementWidget.baseUrl +
        "/enumerations/issue_priorities.json?key=" + taskManagementWidget.loadApiKey());

    if (!statusRes.first || !priorityRes.first) {
        taskManagementWidget.updateStatus(false, "Fetching all of the enumerations failed.");
        return false;
    }
    const auto statusJmap = QJsonDocument::fromJson(statusRes.second).object();
    const auto priorityJmap = QJsonDocument::fromJson(priorityRes.second).object();
    for (auto elt : statusJmap["issue_statuses"].toArray()) {
        auto eltObj = elt.toObject();
        enumerations.statusEnum.append(QPair(eltObj["id"].toInt(), eltObj["name"].toString()));
    }
    for (auto elt : priorityJmap["issue_priorities"].toArray()) {
        auto eltObj = elt.toObject();
        enumerations.priorityEnum.append(QPair(eltObj["id"].toInt(), eltObj["name"].toString()));
    }
    return true;
}

bool TaskSelectionTab::fetchProjects() {
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;

    // we can only get at most 100 projects by request but we want all of them
    QList<QPair<bool, QByteArray>> projectResponses;
    auto limit = 100;
    projectResponses.append(Network::singleton().refresh(
        taskManagementWidget.baseUrl + "/projects.json?limit=" + QString::number(limit) +
        "&key=" + taskManagementWidget.loadApiKey()));
    while (projectResponses.last().first) {
        const auto jMap = QJsonDocument::fromJson(projectResponses.last().second).object();
        const auto offset = limit * projectResponses.size();
        if (jMap["total_count"].toInt() <= offset)
            break;

        projectResponses.append(Network::singleton().refresh(
            taskManagementWidget.baseUrl + "/projects.json?limit=" + QString::number(limit) +
            "&offset=" + QString::number(offset) + "&key=" + taskManagementWidget.loadApiKey()));
    }
    if (!projectResponses.last().first) {
        taskManagementWidget.updateStatus(false, "Fetching all of the projects failed.");
        return false;
    }

    QHash<int, QStandardItem*> id2Item;
    QHash<int, int> id2ParentId;
    for (auto projectRes : projectResponses) {
        const auto projectJMap = QJsonDocument::fromJson(projectRes.second).object();
        for (auto elt : projectJMap["projects"].toArray()) {
            auto eltObj = elt.toObject();
            auto id = eltObj["id"].toInt();
            auto item = new QStandardItem(eltObj["identifier"].toString());
            item->setData(id, Qt::UserRole);
            id2Item[id] = item;
            id2ParentId[id] =
                eltObj.contains("parent") ? eltObj["parent"].toObject()["id"].toInt() : -1;
        }
    }
    for (auto it = id2Item.constBegin(); it != id2Item.constEnd(); ++it) {
        auto parentId = id2ParentId[it.key()];
        // edge case when the user has access to a sub-project but hasn't access to its parent
        if (parentId == -1 || !id2Item.contains(parentId))
            projectItemModel.invisibleRootItem()->appendRow(it.value());
        else
            id2Item[parentId]->appendRow(it.value());
    }

    projectItemModel.sort(0);
    return true;
}

QString TaskSelectionTab::issuesUrlParams() {
    // NOTE: using limit=K with K bigger than 100 doesn't fetch more results
    QString string{"&include=attachments&limit=100"};
    if (assignedToMeCheckBox.isChecked())
        string.append("&assigned_to_id=me");
    if (statusChoiceCombo.currentIndex() > 0)
        string.append(
            "&status_id=" +
            QString::number(enumerations.statusEnum[statusChoiceCombo.currentIndex()].first));
    if (priorityCombo.currentIndex() > 0)
        string.append(
            "&priority_id=" +
            QString::number(enumerations.priorityEnum[priorityCombo.currentIndex()].first));
    if (projectTreeView.currentIndex().isValid())
        string.append("&project_id=" +
                      QString::number(projectItemModel
                                          .itemFromIndex(projectProxyModel.mapToSource(
                                              projectTreeView.currentIndex()))
                                          ->data(Qt::UserRole)
                                          .value<int>()));
    return string;
}

bool TaskSelectionTab::saveAndLoadFile(const QString& filename, const QByteArray& content) {
    QDir taskDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/tasks");
    taskDir.mkpath("./task-files"); // filenames of new tasks contain subfolder

    // Retrieve the filename from response header and rename the previously created tmp.nml
    if (!filename.isEmpty()) {
        const QString actualFilename = taskDir.absolutePath() + "/" + filename;

        QFile tmpFile(actualFilename);
        const auto success = tmpFile.open(QIODevice::WriteOnly) && (tmpFile.write(content) != -1);
        tmpFile.close(); // it’s reopened by openFileDispatch

        if (success && state->viewer->window->openFileDispatch({actualFilename})) {
            return taskManagementWidget.updateStatus(true, "Annotation successfully loaded.");
        } else {
            taskManagementWidget.updateStatus(false, "Loading " + actualFilename + " failed.");
        }
    }
    return false;
}

void TaskSelectionTab::showDefaultColumns() {
    for (int i = 0; i < defaultColumnCount; ++i) {
        taskTreeView.showColumn(i);
    }
    for (int i = defaultColumnCount; i < taskModel.columnCount(); ++i) {
        taskTreeView.hideColumn(i);
    }
    adjustTaskTreeView();
}

bool TaskSelectionTab::resetSearch() {
    enumerations.statusEnum.clear();
    enumerations.priorityEnum.clear();

    enumerations.statusEnum.append(QPair(-1, "All"));
    enumerations.priorityEnum.append(QPair(-1, "All"));

    if (!fetchEnumerations())
        return false;
    statusChoiceCombo.insertItems(1, enumNames(enumerations.statusEnum));
    priorityCombo.insertItems(1, enumNames(enumerations.priorityEnum));

    projectItemModel.clear();
    if (!fetchProjects())
        return false;

    projectSearchBar.clear();
    assignedToMeCheckBox.setChecked(true);
    return true;
}

bool TaskSelectionTab::updateAndRefreshWidget() {
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;

    if (!enumerations.statusEnum.size()) { // search is not initialized
        if (!resetSearch())
            return false;
    }

    auto res = taskModel.fetch(taskManagementWidget.baseUrl + "/issues.json?key=" +
                               taskManagementWidget.loadApiKey() + issuesUrlParams());
    if (!res.first) {
        return taskManagementWidget.updateStatus(false, res.second);
    }
    auto issueCounts = taskModel.issueCounts();
    taskCountLabel.setText(QString::number(issueCounts.first) + " out of " +
                           QString::number(issueCounts.second) + " tasks");
    adjustTaskTreeView();
    return true;
}

void TaskSelectionTab::fetchOpenAttachment(TaskItem* taskItem) {
    if (!Annotation::singleton().isEmpty() && Annotation::singleton().unsavedChanges) {
        if (!loginGuardMsg()) {
            return;
        }
        Annotation::singleton().clearAnnotation();
    }

    auto taskObject = taskModel.json()["issues"].toArray()[taskItem->taskIndex].toObject();
    auto attachmentObject =
        taskObject["attachments"].toArray()[taskItem->attachmentsIndex].toObject();
    auto attachmentPath = QString::number(attachmentObject["id"].toInt()) + "/" +
                          attachmentObject["filename"].toString();

    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;
    const auto res =
        Network::singleton().refresh(taskManagementWidget.baseUrl + "/attachments/download/" +
                                     attachmentPath + "?key=" + taskManagementWidget.loadApiKey());

    if (!res.first) {
        taskManagementWidget.updateStatus(false, "Fetching the attachment failed.");
        return;
    }

    if (saveAndLoadFile(attachmentObject["filename"].toString(), res.second)) {
        emit readyOpenTask(taskObject["id"].toInt());
    };
}
void TaskSelectionTab::onFetchMoreButtonCLick() {
    WidgetDisabler d{
        *this}; // don’t allow widget interaction while Network has an event loop running
    LoadingCursor loadingcursor;

    taskModel.canFetchMoreValue = true;
    taskProxyModel.fetchMore(QModelIndex()); // it does the network request
    taskModel.canFetchMoreValue = false;

    auto issueCounts = taskModel.issueCounts();
    taskCountLabel.setText(QString::number(issueCounts.first) + " out of " +
                           QString::number(issueCounts.second) + " tasks");
}

void TaskSelectionTab::onTaskTreeContextMenu(const QPoint& point) {
    if (!taskTreeView.indexAt(point).isValid())
        return;

    auto index = taskProxyModel.mapToSource(taskTreeView.indexAt(point));
    auto taskItem = static_cast<TaskItem*>(index.internalPointer());
    QMenu menu(this);

    if (taskItem->attachmentsIndex == -1) { // a task
        auto openKnossosAction = menu.addAction("Open task");

        QObject::connect(openKnossosAction, &QAction::triggered, this, [this, taskItem] {
            emit readyOpenTask(
                taskModel.json()["issues"].toArray()[taskItem->taskIndex].toObject()["id"].toInt());
        });
    } else { // an attachment
        auto loadKnossosAction = menu.addAction("Load in KNOSSOS and open task");

        QObject::connect(loadKnossosAction, &QAction::triggered, this,
                         [this, taskItem] { fetchOpenAttachment(taskItem); });
    }

    auto openBrowserAction = menu.addAction("Open in a web browser");
    QObject::connect(openBrowserAction, &QAction::triggered, this, [this, taskItem] {
        QDesktopServices::openUrl(QUrl(taskManagementWidget.baseUrl + "/issues/" +
                                       QString::number(taskModel.json()["issues"]
                                                           .toArray()[taskItem->taskIndex]
                                                           .toObject()["id"]
                                                           .toInt())));
    });

    menu.exec(taskTreeView.mapToGlobal(point));
}

void TaskSelectionTab::onTaskTreeDoubleCLick(const QModelIndex& index) {
    auto src_index = taskProxyModel.mapToSource(index);
    auto taskItem = static_cast<TaskItem*>(src_index.internalPointer());

    if (taskItem->attachmentsIndex != -1) { // an attachment
        fetchOpenAttachment(taskItem);
    }
}

void TaskSelectionTab::onTaskTreeHeaderContextMenu(const QPoint& point) {
    QMenu menu(this);

    auto index = taskTreeView.header()->logicalIndexAt(point);
    if (index != -1) {
        auto visibleCount = 0;
        for (int i = 0; i < taskModel.columnCount(); ++i) {
            if (!taskTreeView.isColumnHidden(i))
                ++visibleCount;
        }
        if (visibleCount > 1) { // prevent user from getting stuck (no column shown => no header)
            auto hideColumnAction = menu.addAction("Hide column");
            QObject::connect(hideColumnAction, &QAction::triggered, [this, index] {
                taskTreeView.hideColumn(index);
                adjustTaskTreeView();
            });
        }
    }

    auto showDefaultColumns = menu.addAction("Show default columns");
    QObject::connect(showDefaultColumns, &QAction::triggered, this,
                     &TaskSelectionTab::showDefaultColumns);

    auto showAllColumns = menu.addAction("Show all columns");
    QObject::connect(showAllColumns, &QAction::triggered, [this] {
        for (int i = 0; i < taskModel.columnCount(); ++i) {
            taskTreeView.showColumn(i);
        }
        adjustTaskTreeView();
    });

    menu.exec(taskTreeView.header()->mapToGlobal(point));
}
