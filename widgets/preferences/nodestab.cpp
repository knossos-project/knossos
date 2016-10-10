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

#include "nodestab.h"
#include "widgets/GuiConstants.h"
#include "viewer.h"
#include "widgets/viewport.h"

#include <QFileDialog>
#include <QMessageBox>

int PropertyModel::rowCount(const QModelIndex &) const {
    return properties.size();
}

QVariant PropertyModel::data(const QModelIndex & index, int role) const {
    if (role == Qt::DisplayRole) {
        return properties[index.row()];
    }
    return QVariant();
}

void PropertyModel::recreate(const QSet<QString> & numberProperties)  {
    beginResetModel();
    properties.clear();
    for (const auto & property : numberProperties) {
        properties.emplace_back(property);
    }
    std::sort(std::begin(properties), std::end(properties));
    properties.insert(std::begin(properties), "none (select property)");
    endResetModel();
}

NodesTab::NodesTab(QWidget *parent) : QWidget(parent) {
    idCombo.addItems({"None", "Active node", "All nodes"});
    overrideNodeRadiusSpin.setSingleStep(0.1);
    overrideNodeRadiusSpin.setSuffix(" px");
    edgeNodeRatioSpin.setSingleStep(0.1);
    propertyModel.recreate({});
    propertyRadiusCombo.setModel(&propertyModel);
    propertyRadiusCombo.setCurrentIndex(0);
    propertyRadiusScaleSpin.setPrefix("×");
    propertyRadiusCombo.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    propertyColorCombo.setModel(&propertyModel);
    propertyColorCombo.setCurrentIndex(0);
    propertyColorCombo.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    propertyMinSpin.setRange(-10e8, 10e8);
    propertyMinSpin.setSingleStep(0.1);
    propertyMinSpin.setPrefix("Min: ");
    propertyMaxSpin.setRange(-10e8, 10e8);
    propertyMaxSpin.setSingleStep(0.1);
    propertyMaxSpin.setPrefix("Max: ");

    generalLayout.setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    generalLayout.setAlignment(Qt::AlignTop);
    generalLayout.addRow(tr("Show node IDs"), &idCombo);
    generalLayout.addRow(&nodeCommentsCheck);
    generalLayout.addRow(&overrideNodeRadiusCheck, &overrideNodeRadiusSpin);
    generalLayout.addRow(tr("Edge:node radius ratio"), &edgeNodeRatioSpin);
    generalGroup.setLayout(&generalLayout);
    int row = 0;
    propertiesLayout.addWidget(&propertyRadiusLabel, row++, 0, 1, 4);
    propertiesLayout.addWidget(&propertyRadiusCombo, row, 0);
    propertiesLayout.addWidget(&propertyRadiusScaleSpin, row++, 1);
    propertiesLayout.addWidget(&propertyColorLabel, row++, 0, 1, 4);
    propertiesLayout.addWidget(&propertyColorCombo, row, 0);
    propertiesLayout.addWidget(&propertyMinSpin, row, 1);
    propertiesLayout.addWidget(&propertyMaxSpin, row, 2);
    propertiesLayout.addWidget(&propertyLUTButton, row++, 3);
    propertiesLayout.addWidget(&lutLabel, row++, 0, 1, 4, Qt::AlignRight);
    propertiesLayout.setColumnStretch(0, 1);
    propertiesGroup.setLayout(&propertiesLayout);
    mainLayout.addWidget(&generalGroup);
    mainLayout.addWidget(&propertiesGroup);
    setLayout(&mainLayout);
    QObject::connect(&idCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [](const int index) {
        state->viewerState->idDisplay = index == 2 ? IdDisplay::AllNodes : index == 1 ? IdDisplay::ActiveNode : IdDisplay::None;
    });
    QObject::connect(&overrideNodeRadiusSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value ) { state->viewerState->overrideNodeRadiusVal = value; });
    QObject::connect(&nodeCommentsCheck, &QCheckBox::clicked, [](const bool checked) { ViewportOrtho::showNodeComments = checked; });
    QObject::connect(&overrideNodeRadiusCheck, &QCheckBox::clicked, [this](const bool on) {
        state->viewerState->overrideNodeRadiusBool = on;
        overrideNodeRadiusSpin.setEnabled(on);
    });
    QObject::connect(&edgeNodeRatioSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->viewerState->segRadiusToNodeRadius = value; });
    // properties
    QObject::connect(&propertyRadiusCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int index) {
        state->viewerState->highlightedNodePropertyByRadius = (index > 0) ? propertyModel.properties[index] : "";
        propertyRadiusScaleSpin.setEnabled(index > 0);
    });
    QObject::connect(&propertyRadiusScaleSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->viewerState->nodePropertyRadiusScale = value; });
    QObject::connect(&propertyColorCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int index) {
        state->viewerState->highlightedNodePropertyByColor = (index > 0) ? propertyModel.properties[index] : "";
        propertyMinSpin.setEnabled(index > 0);
        propertyMaxSpin.setEnabled(index > 0);
        propertyLUTButton.setEnabled(index > 0);
        lutLabel.setEnabled(index > 0);
        if(index > 0) {
            loadNodeLUTRequest(lutPath);
        }
    });
    QObject::connect(&propertyMinSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) { state->viewerState->nodePropertyColorMapMin = value; });
    QObject::connect(&propertyMaxSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) { state->viewerState->nodePropertyColorMapMax = value; });
    QObject::connect(&propertyLUTButton, &QPushButton::clicked, [this]() { loadNodeLUTRequest(); });
}

void NodesTab::updateProperties(const QSet<QString> & numberProperties) {
    const auto radiusText = propertyRadiusCombo.currentText();
    const auto colorText = propertyColorCombo.currentText();
    propertyModel.recreate(numberProperties);
    propertyRadiusCombo.setCurrentText(radiusText);
    propertyColorCombo.setCurrentText(colorText);
}

void NodesTab::loadNodeLUTRequest(QString path) {
    if (path.isEmpty()) {
        state->viewerState->renderInterval = SLOW;
        path = QFileDialog::getOpenFileName(this, "Load node color lookup table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
        state->viewerState->renderInterval = FAST;
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            state->viewer->loadNodeLUT(path);
            lutPath = path;
            lutLabel.setText("Current LUT: " + lutPath);
        }  catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, "LUT loading failed", "LUTs are restricted to 256 RGB tuples", QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
        }
    }
}

void NodesTab::saveSettings(QSettings & settings) const {
    settings.setValue(NODE_ID_DISPLAY, idCombo.currentIndex());
    settings.setValue(EDGE_TO_NODE_RADIUS, edgeNodeRatioSpin.value());
    settings.setValue(OVERRIDE_NODES_RADIUS_CHECKED, overrideNodeRadiusCheck.isChecked());
    settings.setValue(OVERRIDE_NODES_RADIUS_VALUE, overrideNodeRadiusSpin.value());
    settings.setValue(SHOW_NODE_COMMENTS, nodeCommentsCheck.isChecked());
    settings.setValue(NODE_PROPERTY_RADIUS_SCALE, propertyRadiusScaleSpin.value());
    settings.setValue(NODE_PROPERTY_LUT_PATH, lutPath);
    settings.setValue(NODE_PROPERTY_MAP_MIN, propertyMinSpin.value());
    settings.setValue(NODE_PROPERTY_MAP_MAX, propertyMaxSpin.value());
}

void NodesTab::loadSettings(const QSettings & settings) {
    idCombo.setCurrentIndex(settings.value(NODE_ID_DISPLAY, 0).toInt());
    if (idCombo.currentIndex() == -1) {
        idCombo.setCurrentIndex(0);
    }
    edgeNodeRatioSpin.setValue(settings.value(EDGE_TO_NODE_RADIUS, 1.5).toDouble());
    edgeNodeRatioSpin.valueChanged(edgeNodeRatioSpin.value());
    overrideNodeRadiusCheck.setChecked(settings.value(OVERRIDE_NODES_RADIUS_CHECKED, false).toBool());
    overrideNodeRadiusCheck.clicked(overrideNodeRadiusCheck.isChecked());
    overrideNodeRadiusSpin.setEnabled(state->viewerState->overrideNodeRadiusBool);
    overrideNodeRadiusSpin.setValue(settings.value(OVERRIDE_NODES_RADIUS_VALUE, 1.5).toDouble());
    overrideNodeRadiusSpin.valueChanged(overrideNodeRadiusSpin.value());
    edgeNodeRatioSpin.setValue(settings.value(EDGE_TO_NODE_RADIUS, 0.5).toFloat());
    edgeNodeRatioSpin.valueChanged(edgeNodeRatioSpin.value());
    nodeCommentsCheck.setChecked(settings.value(SHOW_NODE_COMMENTS, false).toBool());
    nodeCommentsCheck.clicked(nodeCommentsCheck.isChecked());
    propertyRadiusScaleSpin.setValue(settings.value(NODE_PROPERTY_RADIUS_SCALE, 1).toDouble());
    propertyRadiusScaleSpin.valueChanged(propertyRadiusScaleSpin.value());
    // from http://peterkovesi.com/projects/colourmaps/index.html
    lutPath = settings.value(NODE_PROPERTY_LUT_PATH, ":/resources/color_palette/linear_kry_5-98_c75.json").toString();
    lutLabel.setText("Current LUT: " + lutPath);
    propertyMinSpin.setValue(settings.value(NODE_PROPERTY_MAP_MIN, 0).toDouble());
    propertyMinSpin.valueChanged(propertyMinSpin.value());
    propertyMaxSpin.setValue(settings.value(NODE_PROPERTY_MAP_MAX, 0).toDouble());
    propertyMaxSpin.valueChanged(propertyMaxSpin.value());
}
