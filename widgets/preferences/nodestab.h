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

#include <QAbstractListModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

class PropertyModel : public QAbstractListModel {
    Q_OBJECT
    friend class NodesTab;
    std::vector<QString> properties;
    int numberPropertySize{0};
public:
    PropertyModel();
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex & index, int role) const override;
    void recreate(const QSet<QString> & numberProperties, const QSet<QString> & textProperties);
};

class NodesTab : public QWidget {
    friend class PreferencesWidget;
    Q_OBJECT
    QVBoxLayout mainLayout;
    QGroupBox generalGroup{tr("Rendering")};
    QFormLayout generalLayout;
    QComboBox idCombo;
    QCheckBox nodeCommentsCheck{"Show node comments"};
    QCheckBox overrideNodeRadiusCheck{"Override node radius"};
    QDoubleSpinBox overrideNodeRadiusSpin;
    QDoubleSpinBox edgeNodeRatioSpin;
    // property visualization
    QGroupBox propertiesGroup{tr("Property highlighting")};
    QGridLayout propertiesLayout;
    PropertyModel propertyModel;
    QLabel propertyRadiusLabel{"Scale node radius with property value:"};
    QComboBox propertyRadiusCombo;
    QDoubleSpinBox propertyRadiusScaleSpin;
    QLabel propertyColorLabel{"Map property value to node color:"};
    QComboBox propertyColorCombo;
    QDoubleSpinBox propertyMinSpin;
    QDoubleSpinBox propertyMaxSpin;
    QPushButton propertyMinMaxButton{"Find range"};
    QPushButton propertyLUTButton{"Load color LUT …"};
    QLabel lutLabel{"Current LUT: none"};

    QString lutPath;
    void loadNodeLUTRequest(QString path = "");
    void saveSettings(QSettings &settings) const;
    void loadSettings(const QSettings &settings);
public:
    explicit NodesTab(QWidget *parent = nullptr);
    void updateProperties(const QSet<QString> & properties, const QSet<QString> & textProperties);
};
