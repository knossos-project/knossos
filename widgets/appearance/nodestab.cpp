#include "nodestab.h"

#include "skeleton/skeletonizer.h"
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

void PropertyModel::recreate()  {
    beginResetModel();
    properties.clear();
    properties.emplace_back("none (select property)");
    for (const auto & property : Skeletonizer::singleton().getNumberProperties()) {
        properties.emplace_back(property);
    }
    std::sort(std::begin(properties), std::end(properties));
    endResetModel();
}

NodesTab::NodesTab(QWidget *parent) : QWidget(parent) {
    edgeNodeRatioSpin.setSingleStep(0.1);

    propertyModel.recreate();
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
    mainLayout.addWidget(&allNodeIDsCheck, row++, 0);
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
    QObject::connect(&allNodeIDsCheck, &QCheckBox::toggled, [](const bool on) { state->skeletonState->showNodeIDs = on; });
    QObject::connect(&nodeRadiusSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value ) { state->skeletonState->overrideNodeRadiusVal = value; });
    QObject::connect(&nodeCommentsCheck, &QCheckBox::toggled, [](const bool checked) { Viewport::showNodeComments = checked; });
    QObject::connect(&overrideNodeRadiusCheck, &QCheckBox::toggled, [this](const bool on) {
        state->skeletonState->overrideNodeRadiusBool = on;
        nodeRadiusSpin.setEnabled(on);
    });
    QObject::connect(&edgeNodeRatioSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->skeletonState->segRadiusToNodeRadius = value; });
    // properties
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::nodeChangedSignal, [this](const nodeListElement &) { propertyModel.recreate(); });
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, [this]() {
        const auto radiusText = propertyRadiusCombo.currentText();
        const auto colorText = propertyColorCombo.currentText();
        propertyModel.recreate();
        propertyRadiusCombo.setCurrentText(radiusText);
        propertyColorCombo.setCurrentText(colorText);
    });
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

void NodesTab::loadNodeLUTRequest(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Node Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            Skeletonizer::singleton().loadNodeLUT(path);
            lutPath = path;
            lutLabel.setText("Current LUT: " + lutPath);
        }  catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, "LUT loading failed", "LUTs are restricted to 256 RGB tuples", QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
        }
    }
}
