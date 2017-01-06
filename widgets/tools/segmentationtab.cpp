#include "segmentationtab.h"

#include "viewer.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <algorithm>
#include <chrono>

bool NonRemovableQComboBox::event(QEvent * event) {
    //disable implicit deletion
    if (event->type() == QEvent::DeferredDelete) {
        setParent(nullptr);
        event->accept();
        return true;
    } else {
        return QComboBox::event(event);
    }
}

CategoryDelegate::CategoryDelegate(CategoryModel & categoryModel) {
    box.setModel(&categoryModel);
    box.setEditable(true);//support custom categories
    //we don’t wanna insert values here, they get added in Segmentation::changeCategory
    //ENTER after typing a non-existent value doesn’t work otherwise
    box.setInsertPolicy(QComboBox::NoInsert);
}

QWidget * CategoryDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    box.setParent(parent);//parent is needed for proper placement
    return &box;
}

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
    if(index.column() == 0 && role == Qt::CheckStateRole){
        return (obj.on_off ? Qt::Checked : Qt::Unchecked);
    }else if (index.column() == 1 && role == Qt::BackgroundRole) {
        const auto colorIndex = obj.index % 256;
        const auto red = Segmentation::singleton().overlayColorMap[0][colorIndex];
        const auto green = Segmentation::singleton().overlayColorMap[1][colorIndex];
        const auto blue = Segmentation::singleton().overlayColorMap[2][colorIndex];
        return QColor(red, green, blue);

    } else if (index.column() == 3 && role == Qt::CheckStateRole) {
        return (obj.immutable ? Qt::Checked : Qt::Unchecked);
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {

        //std:: << obj.subobjects.size() << std::endl;
        switch (index.column()) {
        case 2: return static_cast<quint64>(obj.id);
        case 4: return obj.category;
        case 5: return obj.comment;
        case 6: return static_cast<quint64>(obj.subobjects.size());
        case 7: {
            QString output;
            const auto elemCount = std::min(MAX_SHOWN_SUBOBJECTS,obj.subobjects.size());
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
    if (index.column() == 3 && role == Qt::CheckStateRole) {
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
        case 4: Segmentation::singleton().changeCategory(obj, value.toString()); break;
        case 5: Segmentation::singleton().changeComment(obj, value.toString()); break;

        default:
            return false;
        }
    } else if (index.column() == 0 && role == Qt::CheckStateRole){//rutuja

        if(value == Qt::Checked)
        {
          obj.on_off = Qt::Checked;
        }
        else{

          obj.on_off =Qt::Unchecked;
        }
        auto & seg = Segmentation::singleton();
        //Viewer viewer;
        //viewer.run();
        seg.branch_onoff();


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
    case 0 : flags | Qt::ItemIsUserCheckable;
    case 2:

    case 3:
        return flags | Qt::ItemIsUserCheckable;
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
    categoriesCache.clear();
    for (auto & category : Segmentation::singleton().categories) {
        categoriesCache.emplace_back(category);
    }
    std::sort(std::begin(categoriesCache), std::end(categoriesCache));
    endResetModel();
}

int CategoryModel::rowCount(const QModelIndex &) const {
    return categoriesCache.size();
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return categoriesCache[index.row()];
    }
    return QVariant();
}

SegmentationTab::SegmentationTab(QWidget * const parent) : QWidget(parent), categoryDelegate(categoryModel) {
    toolGroup.addButton(&mergeBtn, 0);
    toolGroup.addButton(&paintBtn, 1);
    modeGroup.addButton(&twodBtn, 0);
    modeGroup.addButton(&threedBtn, 1);

    mergeBtn.setCheckable(true);
    paintBtn.setCheckable(true);
    twodBtn.setCheckable(true);
    threedBtn.setCheckable(true);

    mergeBtn.setToolTip(mergeToolTip);
    paintBtn.setToolTip(paintToolTip);
    twodBtn.setToolTip("Only work on one 2D slice.");
    threedBtn.setToolTip("Apply changes on several consecutive slices.");

    mergeBtn.setChecked(true);
    brushRadiusEdit.setValue(Segmentation::singleton().brush.getRadius());
    twodBtn.setChecked(true);

    toolsLayout.addWidget(&showAllChck);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&mergeBtn);
    toolsLayout.addWidget(&paintBtn);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&brushRadiusLabel);
    toolsLayout.addWidget(&brushRadiusEdit);
    toolsLayout.addStretch();
    toolsLayout.addWidget(&twodBtn);
    toolsLayout.addWidget(&threedBtn);
    layout.addLayout(&toolsLayout);

    categoryModel.recreate();
    categoryFilter.setModel(&categoryModel);
    categoryFilter.setEditable(true);
    categoryFilter.lineEdit()->setPlaceholderText("category");
    commentFilter.setPlaceholderText("Filter for comment...");
    showAllChck.setChecked(Segmentation::singleton().renderAllObjs);

    touchedObjsTable.setModel(&touchedObjectModel);
    touchedObjsTable.setAllColumnsShowFocus(true);
    touchedObjsTable.setContextMenuPolicy(Qt::CustomContextMenu);
    touchedObjsTable.setRootIsDecorated(false);
    touchedObjsTable.setSelectionMode(QAbstractItemView::ExtendedSelection);
    touchedObjsTable.setUniformRowHeights(true);//perf hint from doc
    touchedObjsTable.setItemDelegateForColumn(3, &categoryDelegate);

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
    objectsTable.setItemDelegateForColumn(3, &categoryDelegate);

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
    QObject::connect(&Segmentation::singleton().brush, &brush_t::inverseChanged, [this](bool value){
        mergeBtn.setText(value ? "Unmerge" : "Merge");
        mergeBtn.setToolTip(value ? unmergeToolTip : mergeToolTip);
        paintBtn.setText(value ? "Erase" : "Paint");
        paintBtn.setToolTip(value ? eraseToolTip : paintToolTip);
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
            const auto index = Segmentation::singleton().objects.back().index;
            const auto & proxyIndex = objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(objectModel.index(index, 0)));
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
            const auto index = Segmentation::singleton().objects.back().index;
            const auto & proxyIndex = objectProxyModelComment.mapFromSource(objectProxyModelCategory.mapFromSource(objectModel.index(index, 0)));
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
    QObject::connect(&Segmentation::singleton(), &Segmentation::renderAllObjsChanged, &showAllChck, &QCheckBox::setChecked);
    QObject::connect(&Segmentation::singleton(), &Segmentation::categoriesChanged, &categoryModel, &CategoryModel::recreate);

    QObject::connect(&objectsTable, &QTreeView::customContextMenuRequested, [&](QPoint point){contextMenu(objectsTable, point);});
    QObject::connect(&touchedObjsTable, &QTreeView::customContextMenuRequested, [&](QPoint point){contextMenu(touchedObjsTable, point);});
    QObject::connect(objectsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::selectionChanged);
    QObject::connect(touchedObjsTable.selectionModel(), &QItemSelectionModel::selectionChanged, this, &SegmentationTab::touchedObjSelectionChanged);
    QObject::connect(&showAllChck, &QCheckBox::clicked, &Segmentation::singleton(), &Segmentation::setRenderAllObjs);
    QObject::connect(&objectCreateButton, &QPushButton::clicked, [](){Segmentation::singleton().createandselect=true;});

    touchedObjectModel.recreate();
    objectModel.recreate();


    updateLabels();
}

template<typename Func>
void commitSelection(const QItemSelection & selected, const QItemSelection & deselected, Func proxy) {
    //rutuja
    auto & seg = Segmentation::singleton();
    Segmentation::singleton().activeIndices.clear();
    if(selected.indexes().size() != 0)
    {

        for (const auto & index : selected.indexes()) {

            if (index.column() == 2) {

                Segmentation::singleton().activeIndices.emplace_back(proxy(index.row()));
            }
       }
    }
    else{

      size_t size = seg.selectedObjectIndices.size();

      std::vector<uint64_t> temp(size);
      std::vector<uint64_t>::iterator it;
      for(uint64_t  o = 0;o < size; o++)
      {
        temp.at(o) = o;
      }

      for (const auto & index : deselected.indexes()) {

         it = std::find(temp.begin(),temp.end(),proxy(index.row()));
         if(it != temp.end())
         {
             temp.erase(it);
         }

      }
      seg.activeIndices.emplace_back(temp.front());
   }
   /* for (const auto & index : deselected.indexes()) {
        if (index.column() == 2) {//only evaluate id cell
           Segmentation::singleton().unselectObject(proxy(index.row()));
        }
    }*/

}

void commitSelection(const QItemSelection & selected, const QItemSelection & deselected) {
    commitSelection(selected, deselected, [](const int & i){return i;});
}

void SegmentationTab::touchedObjSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {

    if (touchedObjectSelectionProtection) {
        return;
    }
    Segmentation::singleton().blockSignals(true);//prevent ping pong
    if (!QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
        //unselect all previously selected objects
        commitSelection(QItemSelection{}, objectsTable.selectionModel()->selection());
    }
    commitSelection(selected, deselected, [this](const int & i){
        return touchedObjectModel.objectCache[i].get().index;
    });
    Segmentation::singleton().blockSignals(false);
    updateSelection();
    if (selected.length() == 1) {
       // Segmentation::singleton().jumpToObject(touchedObjectModel.objectCache[selected.indexes().front().row()].get().index);
    }
}

void SegmentationTab::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {

    if (objectSelectionProtection) {
        return;
    }

    const auto & proxySelected = objectProxyModelCategory.mapSelectionToSource(objectProxyModelComment.mapSelectionToSource(selected));
    const auto & proxyDeselected = objectProxyModelCategory.mapSelectionToSource(objectProxyModelComment.mapSelectionToSource(deselected));
    Segmentation::singleton().blockSignals(true);//prevent ping pong
    commitSelection(proxySelected, proxyDeselected);
    Segmentation::singleton().blockSignals(false);
    updateTouchedObjSelection();
    //the proxy selection contains ranges for every cell of a line while the source selection consists of _one_ range per line
    if (selected.length() == 1) {
       // Segmentation::singleton().jumpToObject(proxySelected.indexes().front().row());
    }
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
    if (!categoryFilter.currentText().isEmpty()) {
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

void SegmentationTab::updateLabels() {
    objectCountLabel.setText(QString("Objects: %0").arg(Segmentation::singleton().objects.size()));
    subobjectCountLabel.setText(QString("Subobjects: %0").arg(Segmentation::singleton().subobjects.size()));
}

void SegmentationTab::contextMenu(const QAbstractScrollArea & widget, const QPoint & pos) {
    QMenu contextMenu;
    //rutuja  - temporary commented - needs to be fixed
    //QObject::connect(contextMenu.addAction("merge"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::mergeSelectedObjects);
    //QObject::connect(contextMenu.addAction("delete"), &QAction::triggered, &Segmentation::singleton(), &Segmentation::deleteSelectedObjects);
    //contextMenu.exec(widget.viewport()->mapToGlobal(pos));
    //auto & seg = Segmentation::singleton();
    //if(seg.flag_delete)
    //{
      // seg.branch_delete();
    //}
}
