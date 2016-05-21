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
    idCombo.addItems({"None", "Active Node", "All Nodes"});
    edgeNodeRatioSpin.setSingleStep(0.1);

    propertyModel.recreate({});
    propertyRadiusCombo.setModel(&propertyModel);
    propertyRadiusCombo.setCurrentIndex(0);
    propertyRadiusScaleSpin.setPrefix("×");

    propertyColorCombo.setModel(&propertyModel);
    propertyColorCombo.setCurrentIndex(0);
    propertyMinSpin.setSingleStep(0.1);
    propertyMinSpin.setPrefix("Min: ");
    propertyMaxSpin.setSingleStep(0.1);
    propertyMaxSpin.setPrefix("Max: ");

    nodeSeparator.setFrameShape(QFrame::HLine);
    nodeSeparator.setFrameShadow(QFrame::Sunken);
    mainLayout.setAlignment(Qt::AlignTop);
    int row = 0;
    mainLayout.addWidget(&idLabel, row, 0);
    mainLayout.addWidget(&idCombo, row++, 1);
    mainLayout.addWidget(&nodeCommentsCheck, row++, 0);
    mainLayout.addWidget(&overrideNodeRadiusCheck, row, 0);
    mainLayout.addWidget(&nodeRadiusSpin, row++, 1);
    mainLayout.addWidget(&edgeNodeRatioLabel, row, 0);
    mainLayout.addWidget(&edgeNodeRatioSpin, row++, 1);
    mainLayout.addWidget(&propertyHeader, row++, 0);
    mainLayout.addWidget(&nodeSeparator, row++, 0, 1, 4);
    mainLayout.addWidget(&propertyRadiusLabel, row++, 0, 1, 4);
    mainLayout.addWidget(&propertyRadiusCombo, row, 0);
    mainLayout.addWidget(&propertyRadiusScaleSpin, row++, 1);
    mainLayout.addWidget(&propertyColorLabel, row++, 0, 1, 4);
    mainLayout.addWidget(&propertyColorCombo, row, 0);
    mainLayout.addWidget(&propertyMinSpin, row, 1);
    mainLayout.addWidget(&propertyMaxSpin, row, 2);
    mainLayout.addWidget(&propertyLUTButton, row++, 3);
    mainLayout.addWidget(&lutLabel, row++, 0, 1, 4, Qt::AlignRight);
    mainLayout.setColumnStretch(0, 1);
    setLayout(&mainLayout);
    QObject::connect(&idCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [](const int index) {
        state->viewerState->idDisplay = index == 2 ? IdDisplay::AllNodes : index == 1 ? IdDisplay::ActiveNode : IdDisplay::None;
    });
    QObject::connect(&nodeRadiusSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value ) { state->viewerState->overrideNodeRadiusVal = value; });
    QObject::connect(&nodeCommentsCheck, &QCheckBox::clicked, [](const bool checked) { ViewportOrtho::showNodeComments = checked; });
    QObject::connect(&overrideNodeRadiusCheck, &QCheckBox::clicked, [this](const bool on) {
        state->viewerState->overrideNodeRadiusBool = on;
        nodeRadiusSpin.setEnabled(on);
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
        path = QFileDialog::getOpenFileName(this, "Load Node Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
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
    settings.setValue(OVERRIDE_NODES_RADIUS_VALUE, nodeRadiusSpin.value());
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
    nodeRadiusSpin.setEnabled(state->viewerState->overrideNodeRadiusBool);
    nodeRadiusSpin.setValue(settings.value(OVERRIDE_NODES_RADIUS_VALUE, 1.5).toDouble());
    nodeRadiusSpin.valueChanged(nodeRadiusSpin.value());
    edgeNodeRatioSpin.setValue(settings.value(EDGE_TO_NODE_RADIUS, 0.5).toFloat());
    edgeNodeRatioSpin.valueChanged(edgeNodeRatioSpin.value());
    nodeCommentsCheck.setChecked(settings.value(SHOW_NODE_COMMENTS, false).toBool());
    nodeCommentsCheck.clicked(nodeCommentsCheck.isChecked());
    propertyRadiusScaleSpin.setValue(settings.value(NODE_PROPERTY_RADIUS_SCALE, 1).toDouble());
    propertyRadiusScaleSpin.valueChanged(propertyRadiusScaleSpin.value());
    // from http://peterkovesi.com/projects/colourmaps/index.html
    lutPath = settings.value(NODE_PROPERTY_LUT_PATH, ":/resources/color_palette/linear_kry_5-98_c75.json").toString();
    propertyMinSpin.setValue(settings.value(NODE_PROPERTY_MAP_MIN, 0).toDouble());
    propertyMinSpin.valueChanged(propertyMinSpin.value());
    propertyMaxSpin.setValue(settings.value(NODE_PROPERTY_MAP_MAX, 0).toDouble());
    propertyMaxSpin.valueChanged(propertyMaxSpin.value());
}
