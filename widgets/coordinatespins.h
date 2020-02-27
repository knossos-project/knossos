#pragma once

#include "coordinate.h"

#include <QObject>
#include <QPushButton>
#include <QSpinBox>

class CoordinateSpin : public QSpinBox {
    Q_OBJECT
public:
    CoordinateSpin(const QString & prefix, QWidget * parent = nullptr);
    virtual void fixup(QString & input) const override;
    virtual void keyPressEvent(QKeyEvent *event) override;
signals:
    void copySignal();
    void pasteSignal();
};

class CoordinateSpins : public QObject {
    Q_OBJECT
public:
    CoordinateSpin xSpin{"x: "};
    CoordinateSpin ySpin{"y: "};
    CoordinateSpin zSpin{"z: "};
    QPushButton copyButton{QIcon(":/resources/icons/edit-copy.png"), ""};
    QPushButton pasteButton{QIcon(":/resources/icons/edit-paste.png"), ""};

    explicit CoordinateSpins(QObject *parent = nullptr);
    Coordinate get() const;
    void set(const Coordinate & coords);

signals:
    void coordinatesChanged();
};
