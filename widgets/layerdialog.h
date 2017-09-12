#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include "widgets/DialogVisibilityNotify.h"
#include "widgets/Spoiler.h"

#include <QGroupBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QTreeView>
#include <QAbstractItemModel>

class LayerItemModel : public QAbstractListModel {
Q_OBJECT
protected:
    const std::vector<QString> header{"visible", "opacity", "name", "type"};
public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    void addItem();
    void removeItem(const QModelIndexList &indices);
    void moveItem(const QModelIndexList &indices, int offset);
};

class LayerDialogWidget : public DialogVisibilityNotify {
    Q_OBJECT
public:
    explicit LayerDialogWidget(QWidget * parent = 0);

    QVBoxLayout mainLayout;

    Spoiler optionsSpoiler{"layer options"};
    QGridLayout optionsLayout;
    QLabel opacitySliderLabel{"opacity"};
    QSlider opacitySlider{Qt::Horizontal};
    QLabel rangeDeltaSliderLabel{"range delta"};
    QSlider rangeDeltaSlider{Qt::Horizontal};
    QLabel biasSliderLabel{"bias"};
    QSlider biasSlider{Qt::Horizontal};
    QCheckBox linearFilteringCheckBox{"linear filtering"};

    QVBoxLayout layerLayout;
    LayerItemModel itemModel;
    QTreeView treeView;

    QHBoxLayout controlButtonLayout;
    QLabel infoTextLabel{"Hint: use layers to have layers."};
    QToolButton moveUpButton;
    QToolButton moveDownButton;
    QToolButton addLayerButton;
    QToolButton removeLayerButton;
};

#endif // LAYERDIALOG_H
