#ifndef COORDINATEIMPORTWIDGET_H
#define COORDINATEIMPORTWIDGET_H

#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDialog>

class CoordinateImportWidget : public QDialog {
    Q_OBJECT
    QRegularExpression expression{R"regex((?<number>(?:\d+\.)?\d+)(?:\s|,|\t|$))regex"};
    int xIdx, yIdx, zIdx;
    QTextStream * stream;
    QVBoxLayout mainLayout;
    QHBoxLayout radioLayout;
    QHBoxLayout coordSpinLayout;
    QRadioButton is2dRadioButton{"2d coordinates"};
    QRadioButton is3dRadioButton{"3d coordinates"};
    QSpinBox xIdxSpin, yIdxSpin, zIdxSpin;
    QHBoxLayout buttonLayout;
    QPushButton processAllButton{"Process all"};
    void updateDimensions();
    void validateIndices();
public:
    explicit CoordinateImportWidget(QTextStream && coordStream, QWidget * parent = nullptr);
signals:
    void importDone(const int addedNodes, const int ms);
};

#endif // COORDINATEIMPORTWIDGET_H

