#ifndef COORDINATESPINS_H
#define COORDINATESPINS_H

#include "coordinate.h"

#include <QObject>
#include <QPushButton>
#include <QSpinBox>

class CoordinateSpins : public QObject {
    Q_OBJECT
    class CoordinateSpin : public QSpinBox {
    public:
        CoordinateSpin(const QString & prefix, QWidget * parent = nullptr) : QSpinBox(parent) {
            setPrefix(prefix);
            setRange(0, 1000000);//allow min 0 as bogus value, we don’t adjust the max anyway
            setValue(0 + 1);//inintialize for {0, 0, 0}
        }
        virtual void fixup(QString & input) const override {
            input = QString::number(0);//let viewer reset the value
        }
    };

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

#endif // COORDINATESPINS_H
