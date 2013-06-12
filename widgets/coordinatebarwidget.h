#ifndef COORDINATEBARWIDGET_H
#define COORDINATEBARWIDGET_H

#include <QWidget>

class QLabel;
class QSpinBox;
class QPushButton;
class CoordinateBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CoordinateBarWidget(QWidget *parent = 0);
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *xField, *yField, *zField;
    QPushButton *copyButton, *pasteButton;
signals:
    
public slots:
    
};

#endif // COORDINATEBARWIDGET_H
