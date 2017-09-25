#include "layerdialog.h"

#include <QHeaderView>

// TODO: temporary!
struct LayerGuiElement {
    // data
    int id; // or something else
    float rangeDelta;
    float bias;
    bool linearFiltering;

    // gui
    float opacity = 1.0f;
    bool visible = true;
    QString name = "unnamed";
    QString type = "untyped"; // should be ID
};

static std::vector<LayerGuiElement> guiData {
    {0, 0.0f, 0.0f, false, 0.0f, false, "", ""},
    {1, 0.2f, 0.2f, false, 0.225f, false, "a", "b"},
    {2, 0.5f, 0.5f,  true, 0.5f,  true, "something", "else"},
    {3, 1.0f, 1.0f,  true, 1.0f,  true, "more", "things"},
    {4, 0.7f, 0.7f, false, 0.75f, false, "layername", "layertype"},
};

int LayerItemModel::rowCount(const QModelIndex &) const {
    return guiData.size();
}

int LayerItemModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant LayerItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header[section];
    } else {
        return QVariant();
    }
}

QVariant LayerItemModel::data(const QModelIndex &index, int role) const {
    if(index.isValid()) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch(index.column()) {
            case 1: return QString::number(guiData[index.row()].opacity * 100.0f) + (role == Qt::EditRole ? "" : "%");
            case 2: return guiData[index.row()].name;
            case 3: return guiData[index.row()].type;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                return guiData[index.row()].visible ? Qt::Checked : Qt::Unchecked;
            }
        }
    }
    return QVariant();
}

bool LayerItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if(index.isValid()) {
        if (role == Qt::EditRole) {
            switch(index.column()) {
            case 1:
                guiData[index.row()].opacity = value.toFloat() / 100.0f;
                break;
            case 2:
                guiData[index.row()].name = value.toString();
                break;
            case 3:
                guiData[index.row()].type = value.toString();
                break;
            }
        } else if (role == Qt::UserRole) { // raw data setter
            switch(index.column()) {
            case 1:
                guiData[index.row()].opacity = value.toFloat();
                break;
            case 2:
                guiData[index.row()].name = value.toString();
                break;
            case 3:
                guiData[index.row()].type = value.toString();
                break;
            }
        } else if(role == Qt::CheckStateRole) {
            if(index.column() == 0) {
                guiData[index.row()].visible = value.toBool();
            }
        }
    }
    return true;
}

void LayerItemModel::addItem() {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    guiData.emplace_back(LayerGuiElement{});
    endInsertRows();
}

void LayerItemModel::removeItem(const QModelIndexList &indices) {
    for(const auto& index : indices) {
        beginRemoveRows(QModelIndex(), index.row(), index.row());
        guiData.erase(guiData.begin() + index.row());
        endRemoveRows();
    }
}

void LayerItemModel::moveItem(const QModelIndexList &indices, int offset) {
    for(const auto& index : indices) {
        auto row = index.row();
        if(row + offset >= 0 && row + offset < guiData.size()) {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + ((offset > 0) ? offset + 1 : offset)); // because moving is done into between rows
            std::swap(guiData[row], guiData[row + offset]);
            endMoveRows();
        }
    }
}

Qt::ItemFlags LayerItemModel::flags(const QModelIndex &index) const {
    if(index.isValid()) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch(index.column()) {
        case 0:
            flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
            break;
        case 1:
        case 2:
            flags |= Qt::ItemIsEditable;
            break;
        }
        return flags;
    }
    return 0;
}


LayerDialogWidget::LayerDialogWidget(QWidget *parent) : DialogVisibilityNotify(PREFERENCES_WIDGET, parent) {
    setWindowTitle("Layers");

    int row = 0;
    optionsLayout.addWidget(&opacitySliderLabel, row, 0);
    optionsLayout.addWidget(&opacitySlider, row, 1);
    optionsLayout.addWidget(&linearFilteringCheckBox, row, 2);

    optionsLayout.addWidget(&rangeDeltaSliderLabel, ++row, 0);
    optionsLayout.addWidget(&rangeDeltaSlider, row, 1);

    optionsLayout.addWidget(&biasSliderLabel, ++row, 0);
    optionsLayout.addWidget(&biasSlider, row, 1);
    optionsSpoiler.setContentLayout(optionsLayout);

    treeView.setModel(&itemModel);
    treeView.resizeColumnToContents(0);
    treeView.resizeColumnToContents(1);
    treeView.resizeColumnToContents(2);
//    treeView.header()->setSectionResizeMode(QHeaderView::Fixed); // can cause some unwanted column sizes that are unresizable
//    treeView.header()->setSectionsClickable(false);
//    treeView.header()->setSectionsMovable(false);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

    addLayerButton.setIcon(QIcon(":/resources/icons/dialog-ok.png"));
    removeLayerButton.setIcon(QIcon(":/resources/icons/application-exit.png"));

    controlButtonLayout.addWidget(&infoTextLabel);
    controlButtonLayout.addWidget(&moveUpButton);
    controlButtonLayout.addWidget(&moveDownButton);
    controlButtonLayout.addWidget(&addLayerButton);
    controlButtonLayout.addWidget(&removeLayerButton);

    mainLayout.addWidget(&optionsSpoiler);
    mainLayout.addWidget(&treeView);
    mainLayout.addLayout(&controlButtonLayout);
    setLayout(&mainLayout);

    QObject::connect(&addLayerButton, &QToolButton::clicked, [this](){
        itemModel.addItem();
    });

    QObject::connect(&removeLayerButton, &QToolButton::clicked, [this](){
        itemModel.removeItem(treeView.selectionModel()->selectedRows());
    });

    QObject::connect(&moveUpButton, &QToolButton::clicked, [this](){
        itemModel.moveItem(treeView.selectionModel()->selectedRows(), -1);
    });

    QObject::connect(&moveDownButton, &QToolButton::clicked, [this](){
        itemModel.moveItem(treeView.selectionModel()->selectedRows(), 1);
    });

    resize(700, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}
