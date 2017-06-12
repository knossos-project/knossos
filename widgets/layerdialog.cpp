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
    float opacity;
    bool visible;
    QString name;
    QString type; // should be ID
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
            case 1: return QString::number(guiData[index.row()].opacity * 100.0f) + "%";
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
        // set data
    }
    return true;
}

Qt::ItemFlags LayerItemModel::flags(const QModelIndex &index) const {
    if(index.isValid()) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
        switch(index.column()) {
        case 0:
            flags |= Qt::ItemIsUserCheckable;
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
    treeView.header()->setSectionResizeMode(QHeaderView::Fixed);
    treeView.header()->setSectionsClickable(false);
    treeView.header()->setSectionsMovable(false);
    treeView.setRootIsDecorated(false);
    treeView.setUniformRowHeights(true); // for optimization
    treeView.setDragDropMode(QAbstractItemView::InternalMove);

    controlButtonLayout.addWidget(&infoTextLabel);
    controlButtonLayout.addWidget(&moveUpButton);
    controlButtonLayout.addWidget(&moveDownButton);
    controlButtonLayout.addWidget(&addLayerButton);
    controlButtonLayout.addWidget(&removeLayerButton);

    mainLayout.addWidget(&optionsSpoiler);
    mainLayout.addWidget(&treeView);
    mainLayout.addLayout(&controlButtonLayout);

    setLayout(&mainLayout);

    resize(700, 600);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}
