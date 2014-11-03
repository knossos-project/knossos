#ifndef DATASETLOCALWIDGET_H
#define DATASETLOCALWIDGET_H

#include "knossos-global.h"

#include <QCheckBox>
#include <QDialog>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class DatasetLocalWidget : public QDialog {
    Q_OBJECT
public:
    explicit DatasetLocalWidget(QWidget *parent = 0);
    void changeDataset(bool isGUI);
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QComboBox *pathDropdown;
    QPushButton *datasetfileDialog;
    QStringList getRecentPathItems();
    QSpinBox *supercubeEdgeSpin;
protected:
    QLabel *supercubeSizeLabel;
    QPushButton *cancelButton;
    QPushButton *processButton;
    void waitForLoader();

signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetCompression();
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void datasetSwitchZoomDefaults();
    void startLoaderSignal();
public slots:
    void datasetfileDialogClicked();
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETLOCALWIDGET_H
