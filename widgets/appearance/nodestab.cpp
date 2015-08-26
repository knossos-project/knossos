#include "nodestab.h"

#include "skeleton/skeletonizer.h"
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
    for (const auto & property : Skeletonizer::singleton().getNodeProperties()) {
        properties.emplace_back(property);
    }
    std::sort(std::begin(properties), std::end(properties));
    endResetModel();
}

NodesTab::NodesTab(QWidget *parent) : QWidget(parent) {
    edgeNodeRatioSpin.setSingleStep(0.1);

    propertyCombo.setModel(&propertyModel);
    propertyMinSpin.setPrefix("Min: ");
    propertyMaxSpin.setPrefix("Max: ");
    propertyRadiusScaleSpin.setSingleStep(0.1);

    nodeSeparator.setFrameShape(QFrame::HLine);
    nodeSeparator.setFrameShadow(QFrame::Sunken);
    mainLayout.setAlignment(Qt::AlignTop);
    int row = 0;
    mainLayout.addWidget(&allNodeIDsCheck, row++, 0);
    mainLayout.addWidget(&nodeCommentsCheck, row++, 0);
    mainLayout.addWidget(&overrideNodeRadiusCheck, row, 0); mainLayout.addWidget(&nodeRadiusSpin, row++, 1);
    mainLayout.addWidget(&edgeNodeRatioLabel, row, 0);  mainLayout.addWidget(&edgeNodeRatioSpin, row++, 1);
    mainLayout.addWidget(&nodeSeparator, row++, 0, 1, 5);

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
    QObject::connect(&Skeletonizer::singleton(), &Skeletonizer::resetData, [this]() { propertyModel.recreate(); });
    QObject::connect(&propertyUseLutCheck, &QCheckBox::clicked, [this](const bool checked) {
        if (checked) {//load file if none is cached
            loadNodeLUTButtonClicked(lutFilePath);
        } else {
//            Skeletonizer::singleton().loadNodeLUT();
        }
    });
    QObject::connect(&propertyLUTButton, &QPushButton::clicked, [this]() { loadNodeLUTButtonClicked(); });
}

void NodesTab::loadNodeLUTButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Node Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
//            Skeletonizer::singleton().loadNodeLUT(path);
            lutFilePath = path;
            propertyUseLutCheck.setChecked(true);
        }  catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, "LUT loading failed", "LUTs are restricted to 256 RGB tuples", QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            propertyUseLutCheck.setChecked(false);
        }
    } else {
        propertyUseLutCheck.setChecked(false);
    }
}
