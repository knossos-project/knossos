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
#include "widgets/DialogVisibilityNotify.h"
#include "widgets/UserOrientableSplitter.h"
#include "widgets/viewports/viewportbase.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTextDocument>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

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
    QTableWidget tableWidget;
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
    void datasetCellChanged(int row, int col);
    QStringList getRecentPathItems();
    void insertDatasetRow(const QString & dataset, const int pos);
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
