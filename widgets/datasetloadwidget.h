/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "coordinate.h"
#include "dataset.h"
#include "widgets/DialogVisibilityNotify.h"
#include "widgets/UserOrientableSplitter.h"
#include "widgets/viewports/viewportbase.h"

#include <QAbstractListModel>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QTextDocument>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QVBoxLayout>

class DatasetModel : public QAbstractListModel {
Q_OBJECT
public:
    std::vector<QString> datasets;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    virtual QVariant headerData(int, Qt::Orientation, int = Qt::DisplayRole) const override { return {}; };
    virtual int columnCount(const QModelIndex & = QModelIndex()) const override { return 1; };
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    virtual int rowCount(const QModelIndex & = QModelIndex{}) const override;
    void add(const QString & datasetPath);
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex &) override;
    void clear();
};

class ButtonListView : public QListView {
friend class ButtonDelegate;
    Q_OBJECT
    QPersistentModelIndex currentEditedCellIndex;
    QPushButton fileDialogButton{"…"};
    QPushButton deleteButton{"×"};
protected:
    virtual void leaveEvent(QEvent * e) override {
        QListView::leaveEvent(e);
        emit mouseLeft();
    }
public:
    explicit ButtonListView(QWidget * parent = 0);
signals:
    void mouseLeft();
};

class ButtonDelegate : public QStyledItemDelegate {
Q_OBJECT
public:
    explicit ButtonDelegate(ButtonListView * parent = 0) : QStyledItemDelegate(parent) {};
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
};


class FOVSpinBox : public QSpinBox {
private:
    int cubeEdge;
public:
    FOVSpinBox() {
        setCubeEdge(128);
    }
    void setCubeEdge(const int newCubeEdge) {
        cubeEdge = newCubeEdge;
        setRange(cubeEdge * 2, cubeEdge * 1024);
        setSingleStep(cubeEdge * 2);
        auto val = QString::number(value());
        fixup(val);
        setValue(val.toInt());
    }
    virtual QValidator::State validate(QString &input, int &pos) const override {
        if (QSpinBox::validate(input, pos) == QValidator::Invalid) {
            return QValidator::Invalid;
        }
        auto number = valueFromText(input);
        return ((number % cubeEdge == 0) && ((number / cubeEdge) % 2 == 0)) ? QValidator::Acceptable : QValidator::Intermediate;
    }
    virtual void fixup(QString &input) const override {
        auto number = valueFromText(input);
        auto ratio = static_cast<int>(std::floor(static_cast<float>(number) / cubeEdge));
        if (ratio % 2 == 0) {
            input = textFromValue(ratio * cubeEdge);
        } else {
            input = textFromValue((ratio + 1) * cubeEdge);
        }
    }
};

class DatasetLoadWidget : public DialogVisibilityNotify {
    Q_OBJECT

    QVBoxLayout mainLayout;
    UserOrientableSplitter splitter;
    DatasetModel datasetModel;
    ButtonListView tableWidget;
    ButtonDelegate addButtonDelegate{&tableWidget};
    QLabel infoLabel;
    QGroupBox datasetSettingsGroup;
    QFormLayout datasetSettingsLayout;
    FOVSpinBox fovSpin;
    QLabel superCubeSizeLabel;
    QLabel cubeEdgeLabel{"Cubesize"};
    QSpinBox cubeEdgeSpin;
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QLabel reloadRequiredLabel{tr("Reload dataset for changes to take effect.")};
    QHBoxLayout buttonHLayout;
    QPushButton processButton{"Load Dataset"};
    QPushButton cancelButton{"Close"};

    void applyGeometrySettings();
    void datasetCellChanged(const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &);
    QStringList getRecentPathItems();
    Dataset::list_t infos;
    void updateDatasetInfo(const QUrl &url, const QString &info);
public:
    QUrl datasetUrl;//meh

    explicit DatasetLoadWidget(QWidget *parent = 0);
    bool loadDataset(const boost::optional<bool> loadOverlay = boost::none, QUrl path = {}, const bool silent = false);
    void saveSettings();
    void loadSettings();

signals:
    void updateDatasetCompression();
    void datasetChanged();
    void datasetSwitchZoomDefaults();
public slots:
    void adaptMemoryConsumption();
    void processButtonClicked();
};
