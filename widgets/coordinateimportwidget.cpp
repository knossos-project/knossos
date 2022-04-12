#include "coordinateimportwidget.h"
#include "dataset.h"
#include "skeleton/skeletonizer.h"
#include "widgets/mainwindow.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QTime>

CoordinateImportWidget::CoordinateImportWidget(QTextStream && coordStream, QWidget * parent) : QDialog{parent} {
    stream = &coordStream;

    radioLayout.addWidget(&is2dRadioButton);
    radioLayout.addWidget(&is3dRadioButton);
    const bool is2d = Dataset::current().boundary.z == 1;
    is2dRadioButton.setChecked(is2d);
    is3dRadioButton.setChecked(!is2d);
    updateDimensions();
    processAllButton.setDisabled(true);
    buttonLayout.addWidget(&processAllButton);

    xIdxSpin.setPrefix("x column:");
    yIdxSpin.setPrefix("y column:");
    zIdxSpin.setPrefix("z column:");
    for (auto * spin : {&xIdxSpin, &yIdxSpin, &zIdxSpin}) {
        spin->setRange(0, 100000000);
    }
    coordSpinLayout.addWidget(&xIdxSpin);
    coordSpinLayout.addWidget(&yIdxSpin);
    coordSpinLayout.addWidget(&zIdxSpin);

    mainLayout.addLayout(&radioLayout);
    mainLayout.addLayout(&coordSpinLayout);
    mainLayout.addLayout(&buttonLayout);
    setLayout(&mainLayout);

    QObject::connect(&xIdxSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CoordinateImportWidget::validateIndices);
    QObject::connect(&yIdxSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CoordinateImportWidget::validateIndices);
    QObject::connect(&zIdxSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CoordinateImportWidget::validateIndices);
    QObject::connect(&is2dRadioButton, &QRadioButton::clicked, this, &CoordinateImportWidget::updateDimensions);
    QObject::connect(&is3dRadioButton, &QRadioButton::clicked, this, &CoordinateImportWidget::updateDimensions);
    QObject::connect(this, &CoordinateImportWidget::importDone, parent, [parent] (const int addedNodes, const int ms) {
        const auto elapsed = QTime::fromMSecsSinceStartOfDay(ms).toString(Qt::ISODateWithMs);
        QMessageBox doneMsgBox{parent};
        doneMsgBox.setIcon(QMessageBox::Information);
        doneMsgBox.setText(tr("Added %1 nodes in %2.").arg(addedNodes).arg(elapsed));
        doneMsgBox.exec();
        qDebug() << "Added" << addedNodes << "nodes in" << elapsed;
    });
    QObject::connect(&processAllButton, &QPushButton::clicked, this, [this] {
        LoadingCursor loadingCursor;
        int addedNodes = 0;
        QString line;
        QElapsedTimer timer;
        timer.start();
        {
        QSignalBlocker blocker{Skeletonizer::singleton()};
        std::optional<std::reference_wrapper<treeListElement>> tree = std::nullopt;
        while (!(line = stream->readLine()).isNull()) {
            auto iterator = expression.globalMatch(line);
            std::optional<float> x = std::nullopt, y = std::nullopt, z = is3dRadioButton.isChecked() ? std::nullopt : std::make_optional(0);
            for (int idx = 0; iterator.hasNext(); idx++) {
                if (x.has_value() && y.has_value() && z.has_value()) {
                    break;
                }
                const auto match = iterator.next();
                if (idx == xIdxSpin.value()) {
                    x = match.captured("number").toFloat();
                } else if (idx == yIdxSpin.value()) {
                    y = match.captured("number").toFloat();
                } else if (!z.has_value() && (idx == zIdxSpin.value())) {
                    z = match.captured("number").toFloat();
                }
            }
            if (x.has_value() && y.has_value() && z.has_value()) {
                if (!tree.has_value()) {
                    tree = Skeletonizer::singleton().addTree();
                }
                floatCoordinate pos{x.value(), y.value(), z.value()};
                Skeletonizer::singleton().addNode(boost::none, pos, tree.value(), {});
                addedNodes += 1;
            }
        }
        }
        emit Skeletonizer::singleton().resetData();
        emit importDone(addedNodes, timer.elapsed());
        close();
    });
}

void CoordinateImportWidget::updateDimensions() {
    zIdxSpin.setEnabled(is3dRadioButton.isChecked());
}

void CoordinateImportWidget::validateIndices() {
    const int zColumn = is3dRadioButton.isChecked()? zIdxSpin.value() : -1;
    processAllButton.setEnabled(xIdxSpin.value() != yIdxSpin.value() && xIdxSpin.value() != zColumn
        && yIdxSpin.value() != zColumn);
}
