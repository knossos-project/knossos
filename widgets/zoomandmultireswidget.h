#ifndef ZOOMANDMULTIRESWIDGET_H
#define ZOOMANDMULTIRESWIDGET_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QCheckBox>
#include <QDebug>
#include "knossos-global.h"
#include "viewer.h"

class ZoomAndMultiresWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ZoomAndMultiresWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    void zoomDefaultsSlot();
    void lockDatasetMagSlot(bool on);
    void orthogonalSliderSlot(int value);
    void skeletonSliderSlot(int value);
    void orthogonalSpinBoxSlot(double value);
    void skeletonSpinBoxSlot(double value);
protected:
    void closeEvent(QCloseEvent *event);
private:
    // top layout
    QLabel *orthogonalDataViewportLabel;
    QLabel *skeletonViewLabel;
    QSlider *orthogonalDataViewportSlider;
    QSlider *skeletonViewSlider;
    QDoubleSpinBox *orthogonalDataViewportSpinBox;
    QDoubleSpinBox *skeletonViewSpinBox;

    QPushButton *zoomDefaultsButton;

    // bottom layout
    QLabel *lockDatasetLabel;
    QCheckBox *lockDatasetCheckBox;
    QLabel *currentActiveMagDatasetLabel;
    QLabel *highestActiveMagDatasetLabel;
    QLabel *lowestActiveMagDatasetLabel;
};

#endif // ZOOMANDMULTIRESWIDGET_H
