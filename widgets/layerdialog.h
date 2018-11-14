#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include "dataset.h"
#include "widgets/datasetloadwidget.h"
#include "widgets/DialogVisibilityNotify.h"
#include "widgets/Spoiler.h"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QSlider>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QSpinBox>

class LayerItemModel : public QAbstractListModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"visible", "opacity", "cubetype", "api", "mag", "edgelength", "experiment", "description", "color"};

public:
    LayerItemModel();
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    void addItem();
    void moveItem(const QModelIndex &indices, int offset);
    void reset();

    std::size_t ordered_i(std::size_t index) const;
};

class LayerDialogWidget : public DialogVisibilityNotify {
Q_OBJECT
public:
    explicit LayerDialogWidget(QWidget * parent = nullptr);

    QVBoxLayout mainLayout;

    Spoiler optionsSpoiler{"layer options"};
    QGridLayout optionsLayout;
    QLabel opacitySliderLabel{"opacity"};
    QSlider opacitySlider{Qt::Horizontal};
    QLabel rangeDeltaSliderLabel{"range delta"};
    QSlider rangeDeltaSlider{Qt::Horizontal};
    QLabel biasSliderLabel{"bias"};
    QSlider biasSlider{Qt::Horizontal};
    QCheckBox combineSlicesCheck{"combine consecutive slices"};
    QComboBox combineSlicesType;
    QSpinBox combineSlicesSpin;
    QCheckBox combineSlicesXyOnlyCheck{"only in xy"};
    QCheckBox linearFilteringCheckBox{"linear filtering"};

    QVBoxLayout layerLayout;
    LayerItemModel itemModel;
    QTreeView treeView;
    QColorDialog colorDialog;

    QHBoxLayout controlButtonLayout;
    QToolButton moveUpButton;
    QToolButton moveDownButton;
    QToolButton dupLayerButton;
    QToolButton addLayerButton;
    QToolButton removeLayerButton;

    void loadSettings();
    void saveSettings();

private:
    void updateLayerProperties();
};

#endif // LAYERDIALOG_H
