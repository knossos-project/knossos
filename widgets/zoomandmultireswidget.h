#ifndef ZOOMANDMULTIRESWIDGET_H
#define ZOOMANDMULTIRESWIDGET_H

#include <QDialog>



class QSlider;
class QLabel;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
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
public:
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
