/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef NODESTAB_H
#define NODESTAB_H

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
public:
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex & index, int role) const override;
    void recreate(const QSet<QString> & numberProperties);
};

class NodesTab : public QWidget
{
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
    QComboBox propertyRadiusCombo;
    QLabel propertyRadiusLabel{"Use scaled property as node radius:"};
    QDoubleSpinBox propertyRadiusScaleSpin;
    QComboBox propertyColorCombo;
    QDoubleSpinBox propertyMinSpin;
    QDoubleSpinBox propertyMaxSpin;
    QLabel propertyColorLabel{"Map property to node color:"};
    QString lutPath;
    QLabel lutLabel{"Current LUT: none"};
    QPushButton propertyLUTButton{"Load color LUT …"};
    void loadNodeLUTRequest(QString path = "");
    void saveSettings(QSettings &settings) const;
    void loadSettings(const QSettings &settings);
public:
    explicit NodesTab(QWidget *parent = 0);
    void updateProperties(const QSet<QString> & numberProperties);
signals:

public slots:
};

#endif // NODESTAB_H
