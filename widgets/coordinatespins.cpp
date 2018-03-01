#include "coordinatespins.h"

#include <QApplication>
#include <QClipboard>

CoordinateSpins::CoordinateSpins(QObject * parent) : QObject(parent) {
    copyButton.setMinimumSize(25, 25);
    copyButton.setMaximumSize(copyButton.minimumSize());
    copyButton.setToolTip("Copy");
    pasteButton.setMinimumSize(25, 25);
    pasteButton.setMaximumSize(pasteButton.minimumSize());
    pasteButton.setToolTip("Paste");

    QObject::connect(&xSpin, &CoordinateSpin::editingFinished, this, &CoordinateSpins::coordinatesChanged);
    QObject::connect(&ySpin, &CoordinateSpin::editingFinished, this, &CoordinateSpins::coordinatesChanged);
    QObject::connect(&zSpin, &CoordinateSpin::editingFinished, this, &CoordinateSpins::coordinatesChanged);

    QObject::connect(&copyButton, &QPushButton::clicked, [this](const bool) {
        const auto content = QString("%1, %2, %3").arg(xSpin.value()).arg(ySpin.value()).arg(zSpin.value());
        QApplication::clipboard()->setText(content);
    });

    QObject::connect(&pasteButton, &QPushButton::clicked, [this](const bool) {
        const QString clipboardContent = QApplication::clipboard()->text();

        const QRegularExpression clipboardRegEx{R"regex((\d+)\D+(\d+)\D+(\d+))regex"};//match 3 groups of digits separated by non-digits
        const auto match = clipboardRegEx.match(clipboardContent);
        if (match.hasMatch()) {// also fails if clipboard is empty
            const auto x = match.captured(1).toInt();// index 0 is the whole matched text
            const auto y = match.captured(2).toInt();
            const auto z = match.captured(3).toInt();
            set({x, y, z});
        } else {
            qDebug() << "Unable to extract coordinates from clipboard content" << clipboardContent;
        }
        emit coordinatesChanged();
    });
}

Coordinate CoordinateSpins::get() const {
    return { xSpin.value(), ySpin.value(), zSpin.value() };
}

void CoordinateSpins::set(const Coordinate & coords) {
    xSpin.setValue(coords.x);
    ySpin.setValue(coords.y);
    zSpin.setValue(coords.z);
}
