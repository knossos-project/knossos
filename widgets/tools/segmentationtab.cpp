#include "segmentationtab.h"

#include "knossos-global.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QString>

#include <chrono>

int TouchedObjectModel::rowCount(const QModelIndex &) const {
    return objectCache.size();
}

QVariant TouchedObjectModel::data(const QModelIndex & index, int role) const {
    if (index.isValid()) {
        //http://coliru.stacked-crooked.com/a/98276b01d551fb41
        const auto & obj = objectCache[index.row()].get();
        return objectGet(obj, index, role);
    }
    return QVariant();//return invalid QVariant
}

bool TouchedObjectModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        auto & obj = objectCache[index.row()].get();
        return objectSet(obj, index, value, role);
    }
    return true;
}

void TouchedObjectModel::recreate() {
    beginResetModel();
    objectCache = Segmentation::singleton().touchedObjects();
    endResetModel();
}

int SegmentationObjectModel::rowCount(const QModelIndex &) const {
    return Segmentation::singleton().objects.size();
}

int SegmentationObjectModel::columnCount(const QModelIndex &) const {
    return header.size();
}

QVariant SegmentationObjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header[section];
    } else {
        return QVariant();//return invalid QVariant
    }
}

QVariant SegmentationObjectModel::objectGet(const Segmentation::Object &obj, const QModelIndex & index, int role) const {
    if (index.column() == 0 && role == Qt::BackgroundRole) {
        const auto colorIndex = obj.id % 256;
        const auto red = Segmentation::singleton().overlayColorMap[0][colorIndex];
        const auto green = Segmentation::singleton().overlayColorMap[1][colorIndex];
        const auto blue = Segmentation::singleton().overlayColorMap[2][colorIndex];
        return QColor(red, green, blue);
    } else if (index.column() == 2 && role == Qt::CheckStateRole) {
        return (obj.immutable ? Qt::Checked : Qt::Unchecked);
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 1: return static_cast<quint64>(obj.id);
        case 3: return obj.category;
        case 4: return obj.comment;
        case 5: return static_cast<quint64>(obj.subobjects.size());
        case 6: {
            QString output;
            const auto elemCount = std::min(MAX_SHOWN_SUBOBJECTS, obj.subobjects.size());
            auto subobjectIt = std::begin(obj.subobjects);
            for (std::size_t i = 0; i < elemCount; ++i) {
                output += QString::number(subobjectIt->get().id) + ", ";
                subobjectIt = std::next(subobjectIt);
            }
            output.chop(2);
            return output;
        }
        }
    }
    return QVariant();//return invalid QVariant
}

QVariant SegmentationObjectModel::data(const QModelIndex & index, int role) const {
    if (index.isValid()) {
        const auto & obj = Segmentation::singleton().objects[index.row()];
        return objectGet(obj, index, role);
    }
    return QVariant();//return invalid QVariant
}

bool SegmentationObjectModel::objectSet(Segmentation::Object & obj, const QModelIndex & index, const QVariant & value, int role) {
    if (index.column() == 2 && role == Qt::CheckStateRole) {
        if (!obj.immutable) {//don’t remove immutability
            QMessageBox prompt;
            prompt.setWindowFlags(Qt::WindowStaysOnTopHint);
            prompt.setIcon(QMessageBox::Question);
            prompt.setWindowTitle(tr("Lock Object"));
            prompt.setText(tr("Lock the object with id %0?").arg(obj.id));
            const auto & lockButton = prompt.addButton(tr("Lock"), QMessageBox::YesRole);
            prompt.addButton(tr("Cancel"), QMessageBox::NoRole);
            prompt.exec();
            if (prompt.clickedButton() == lockButton) {
                obj.immutable = value.toBool();
            }
        }
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 3: obj.category = value.toString(); break;
        case 4: obj.comment  = value.toString(); break;
        default:
            return false;
        }
    }
    return true;
}

bool SegmentationObjectModel::setData(const QModelIndex & index, const QVariant & value, int role) {
    if (index.isValid()) {
        auto & obj = Segmentation::singleton().objects[index.row()];
        return objectSet(obj, index, value, role);
    }
    return true;
}

Qt::ItemFlags SegmentationObjectModel::flags(const QModelIndex & index) const {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemNeverHasChildren;//not editable
    switch(index.column()) {
    case 2:
        return flags | Qt::ItemIsUserCheckable;
    case 3:
    case 4:
        return flags | Qt::ItemIsEditable;
    }
    return flags;
}

void SegmentationObjectModel::recreate() {
    beginResetModel();
    endResetModel();
}

void SegmentationObjectModel::appendRowBegin() {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
}

void SegmentationObjectModel::popRowBegin() {
    beginRemoveRows(QModelIndex(), rowCount()-1, rowCount()-1);
}

void SegmentationObjectModel::appendRow() {
    endInsertRows();
}

void SegmentationObjectModel::popRow() {
    endRemoveRows();
}

void SegmentationObjectModel::changeRow(int id) {
    emit dataChanged(index(id, 0), index(id, columnCount()-1));
}

void CategoryModel::recreate() {
    beginResetModel();
    categories.clear();
    categories.emplace_back("<category>");
    for (auto & category : Segmentation::singleton().categories) {
        categories.emplace_back(category);
    }
    endResetModel();
}

int CategoryModel::rowCount(const QModelIndex &) const {
    return categories.size();
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const {
    if(role == Qt::DisplayRole) {
        return categories[index.row()];
    }
    return QVariant();
}

SegmentationTab::SegmentationTab(QWidget * const parent) : QWidget(parent) {
    toolGroup.addButton(&mergeBtn, 0);
    toolGroup.addButton(&addBtn, 1);
    toolGroup.addButton(&eraseBtn, 2);
    modeGroup.addButton(&twodBtn, 0);
    modeGroup.addButton(&threedBtn, 1);

    mergeBtn.setCheckable(true);
    addBtn.setCheckable(true);
    eraseBtn.setCheckable(true);
    twodBtn.setCheckable(true);
    threedBtn.setCheckable(true);

    mergeBtn.setChecked(true);
    brushRadiusEdit.setValue(Segmentation::singleton().brush.getRadius());
    twodBtn.setChecked(true);

    toolsLayout.addWidget(&showAllChck);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&mergeBtn);
    toolsLayout.addWidget(&addBtn);
    toolsLayout.addWidget(&eraseBtn);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&brushRadiusLabel);
    toolsLayout.addWidget(&brushRadiusEdit);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&twodBtn);
    toolsLayout.addWidget(&threedBtn);
    layout.addLayout(&toolsLayout);

    categoryFilter.setModel(&categoryModel);
    categoryModel.recreate();
    categoryFilter.setCurrentIndex(0);
    commentFilter.setPlaceholderText("Filter for comment...");
    showAllChck.setChecked(Segmentation::singleton().renderAllObjs);

    touchedObjsTable.setModel(&touchedObjectModel);
    touchedObjsTable.setAllColumnsShowFocus(true);
    touchedObjsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    touchedObjsTable.setRootIsDecorated(false);
    touchedObjsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    touchedObjsTable.setUniformRowHeights(true);//perf hint from doc

    //proxy model chaining, so we can filter twice
    objectProxyModelCategory.setSourceModel(&objectModel);
    objectProxyModelComment.setSourceModel(&objectProxyModelCategory);
    objectProxyModelCategory.setFilterKeyColumn(3);
    objectProxyModelComment.setFilterKeyColumn(4);
    objectsTable.setModel(&objectProxyModelComment);
    objectsTable.setAllColumnsShowFocus(true);
    objectsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    objectsTable.setRootIsDecorated(false);
    objectsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    objectsTable.setUniformRowHeights(true);//perf hint from doc
    //sorting seems pretty slow concerning selection and scroll to
//    objectsTable.setSortingEnabled(true);
//    objectsTable.sortByColumn(1, Qt::DescendingOrder);

    filterLayout.addWidget(&categoryFilter);
    filterLayout.addWidget(&commentFilter);
    filterLayout.addWidget(&regExCheckbox);

    bottomHLayout.addWidget(&objectCountLabel);
    bottomHLayout.addWidget(&subobjectCountLabel);
    bottomHLayout.addWidget(&objectCreateButton, 0, Qt::AlignRight);

    splitter.setOrientation(Qt::Vertical);
    splitter.addWidget(&touchedObjsTable);
    splitter.addWidget(&objectsTable);
    layout.addLayout(&filterLayout);
    layout.addWidget(&splitter);
    layout.addLayout(&bottomHLayout);
    setLayout(&layout);

    QObject::connect(&toolGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [](int id){
        Segmentation::singleton().brush.setTool(static_cast<brush_t::tool_t>(id));
    });
    QObject::connect(&brushRadiusEdit, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](int value){
        Segmentation::singleton().brush.setRadius(value);
    });
    QObject::connect(&modeGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [](int id){
        Segmentation::singleton().brush.setMode(static_cast<brush_t::mode_t>(id));
    });
    QObject::connect(&Segmentation::singleton().brush, &brush_t::modeChanged, [this](brush_t::mode_t value){
        modeGroup.button(static_cast<int>(value))->setChecked(true);
    });
    QObject::connect(&Segmentation::singleton().brush, &brush_t::radiusChanged, [this](int value){
        brushRadiusEdit.setValue(value);
    });
    QObject::connect(&Segmentation::singleton().brush, &brush_t::toolChanged, [this](brush_t::tool_t value){
        toolGroup.button(static_cast<int>(value))->setChecked(true);
    });

    QObject::connect(&categoryFilter,  &QComboBox::currentTextChanged, this, &SegmentationTab::filter);
    QObject::connect(&commentFilter, &QLineEdit::textEdited, this, &SegmentationTab::filter);
    QObject::connect(&regExCheckbox, &QCheckBox::stateChanged, this, &SegmentationTab::filter);

    for (const auto & index : {0, 1, 2}) {
        //resize once, constantly resizing slows down selection and scroll to considerably
        touchedObjsTable.resizeColumnToContents(index);
        objectsTable.resizeColumnToContents(index);
    }

    QObject::connect(&Segmentation::singleton(), &Segmentation::beforeAppendRow, &objectModel, &SegmentationObjectModel::appendRowBegin);
    QObject::connect(&Segmentation::singleton(), &Segmentation::beforeRemoveRow, [this](){
        objectSelectionProtection = true;
        objectModel.popRowBegin();
        if (Segmentation::singleton().objects.back().selected) {
            const auto id = Segmentation::singleton().objects.back().id;
            const auto & proxyIndex = objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(objectModel.index(id, 0)));
            objectsTable.selectionModel()->select(proxyIndex, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
        }
        objectSelectionProtection = false;
        touchedObjectModel.recreate();
        updateTouchedObjSelection();
        updateLabels();
    });
    QObject::connect(&Segmentation::singleton(), &Segmentation::appendedRow, [this](){
        objectSelectionProtection = true;
        objectModel.appendRow();
        if (Segmentation::singleton().objects.back().selected) {
            const auto id = Segmentation::singleton().objects.back().id;
            const auto & proxyIndex = objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(objectModel.index(id, 0)));
            objectsTable.selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        objectSelectionProtection = false;
        touchedObjectModel.recreate();
        updateTouchedObjSelection();
        updateLabels();
    });
    QObject::connect(&Segmentation::singleton(), &Segmentation::removedRow, [this](){
        objectModel.popRow();
        touchedObjectModel.recreate();
        updateTouchedObjSelection();
        updateLabels();
    });
    QObject::connect(&Segmentation::singleton(), &Segmentation::changedRow, [this](int id){
        objectSelectionProtection = true;
        objectModel.changeRow(id);
        const auto & proxyIndex = objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(objectModel.index(id, 0)));
        if (Segmentation::singleton().objects[id].selected) {
            objectsTable.selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        } else {
            objectsTable.selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
        }
        objectSelectionProtection = false;
        touchedObjectModel.recreate();
        updateTouchedObjSelection();
        updateLabels();
    });
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetData, [this](){
        touchedObjsTable.clearSelection();
        touchedObjectModel.recreate();
        objectsTable.clearSelection();
        objectModel.recreate();
        updateSelection();
        updateTouchedObjSelection();
        updateLabels();
    });
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetTouchedObjects, &touchedObjectModel, &TouchedObjectModel::recreate);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetTouchedObjects, this, &SegmentationTab::updateTouchedObjSelection);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetSelection, this, &SegmentationTab::updateSelection);
    QObject::connect(&Segmentation::singleton(), &Segmentation::resetSelection, this, &SegmentationTab::updateTouchedObjSelection);

    QObject::connect(&objectsTable, &QTreeView::customContextMenuRequested, [&](QPoint point){contextMenu(objectsTable, point);});
    QObject::connect(&touchedObjsTable, &QTreeView::customContextMenuRequested, [&](QPoint point){contextMenu(touchedObjsTable, point);});
    QObject::connect(objectsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::selectionChanged);
    QObject::connect(touchedObjsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::touchedObjSelectionChanged);
    QObject::connect(&showAllChck, &QCheckBox::clicked, [&](bool value){
        Segmentation::singleton().renderAllObjs = value;
    });

    QObject::connect(&objectCreateButton, &QPushButton::clicked, [](){Segmentation::singleton().createObject();});

    touchedObjectModel.recreate();
    objectModel.recreate();

    updateLabels();
}

void SegmentationTab::commitSelection(const QItemSelection & selected, const QItemSelection & deselected) {
    for (const auto & index : deselected.indexes()) {
        if (index.column() == 1) {//only evaluate id cell
            Segmentation::singleton().unselectObject(index.data().toInt());
        }
    }
    for (const auto & index : selected.indexes()) {
        if (index.column() == 1) {//only evaluate id cell
            Segmentation::singleton().selectObject(index.data().toInt());
        }
    }
}

void SegmentationTab::touchedObjSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
    if (touchedObjectSelectionProtection) {
        return;
    }
    Segmentation::singleton().blockSignals(true);//prevent ping pong
    if (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        //unselect all selected objects which are not present in the touchedObjectsTable
        auto selection = touchedObjsTable.selectionModel()->selection();
        commitSelection(selection, objectsTable.selectionModel()->selection());
    }
    commitSelection(selected, deselected);
    Segmentation::singleton().blockSignals(false);
    updateSelection();
}

void SegmentationTab::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
    if (objectSelectionProtection) {
        return;
    }
    Segmentation::singleton().blockSignals(true);//prevent ping pong
    commitSelection(selected, deselected);
    Segmentation::singleton().blockSignals(false);
    updateTouchedObjSelection();
}

template<typename Elem>
Elem & getElem(Elem & elem) {
    return elem;
}
template<typename Elem>
Elem & getElem(std::reference_wrapper<Elem> & elem) {
    return elem.get();
}

template<typename T>
QItemSelection blockSelection(const SegmentationObjectModel & model, T & data) {
    QItemSelection selectedItems;

    bool blockSelection = false;
    std::size_t blockStartIndex;

    std::size_t objIndex = 0;
    for (auto & elem : data) {
        if (!blockSelection && getElem(elem).selected) { //start block selection
            blockSelection = true;
            blockStartIndex = objIndex;
        }
        if (blockSelection && !getElem(elem).selected) { //end block selection
            selectedItems.select(model.index(blockStartIndex, 0), model.index(objIndex-1, model.columnCount()-1));
            blockSelection = false;
        }
        ++objIndex;
    }
    //finish last blockselection – if any
    if (blockSelection) {
        selectedItems.select(model.index(blockStartIndex, 0), model.index(objIndex-1, model.columnCount()-1));
    }

    return selectedItems;
}

void SegmentationTab::updateTouchedObjSelection() {
    const auto & selectedItems = blockSelection(touchedObjectModel, touchedObjectModel.objectCache);

    touchedObjectSelectionProtection = true;//using block signals prevents update of the tableview
    touchedObjsTable.selectionModel()->select(selectedItems, QItemSelectionModel::ClearAndSelect);
    touchedObjectSelectionProtection = false;
}

void SegmentationTab::updateSelection() {
    const auto & selectedItems = blockSelection(objectModel, Segmentation::singleton().objects);
    const auto & proxySelection = objectProxyModelComment.mapSelectionFromSource(objectProxyModelCategory.mapSelectionFromSource(selectedItems));

    objectSelectionProtection = true;//using block signals prevents update of the tableview
    objectsTable.selectionModel()->select(proxySelection, QItemSelectionModel::ClearAndSelect);
    objectSelectionProtection = false;

    if (!selectedItems.indexes().isEmpty()) {// scroll to first selected entry
        objectsTable.scrollTo(objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(selectedItems.indexes().front())));
    }
}

void SegmentationTab::filter() {
    if (categoryFilter.currentText() != "<category>") {
        objectProxyModelCategory.setFilterFixedString(categoryFilter.currentText());
    } else {
        objectProxyModelCategory.setFilterFixedString("");
    }
    if (regExCheckbox.isChecked()) {
        objectProxyModelComment.setFilterRegExp(commentFilter.text());
    } else {
        objectProxyModelComment.setFilterFixedString(commentFilter.text());
    }
    updateSelection();
    updateTouchedObjSelection();
}

void SegmentationTab::updateCategories() {
    categoryModel.recreate();
    categoryFilter.setCurrentIndex(0);
}

void SegmentationTab::updateLabels() {
    objectCountLabel.setText(QString("Objects: %0").arg(Segmentation::singleton().objects.size()));
    subobjectCountLabel.setText(QString("Subobjects: %0").arg(Segmentation::singleton().subobjects.size()));
}

void SegmentationTab::contextMenu(const QAbstractScrollArea & widget, const QPoint & pos) {
    QMenu contextMenu;
    QObject::connect(contextMenu.addAction("merge"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::mergeSelectedObjects);
    QObject::connect(contextMenu.addAction("delete"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::deleteSelectedObjects);
    contextMenu.exec(widget.viewport()->mapToGlobal(pos));
}

