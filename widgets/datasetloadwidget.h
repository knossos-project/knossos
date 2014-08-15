#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include <QCheckBox>
#include <QDialog>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class DatasetLoadWidget : public QDialog {
    Q_OBJECT
public:
    explicit DatasetLoadWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();
    void changeDataSet(bool isGUI);

protected:
    QGroupBox *localGroup;
    QComboBox *path;
    QPushButton *datasetfileDialog;
    QSpinBox *supercubeEdgeSpin;
    QLabel *supercubeSizeLabel;
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QPushButton *cancelButton;
    QPushButton *processButton;
    void closeEvent(QCloseEvent *event);
    void waitForLoader();
    QStringList getRecentDirsItems();
signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void userMoveSignal(int x, int y, int z);
    void datasetSwitchZoomDefaults();
    void startLoaderSignal();
public slots:
    void datasetfileDialogClicked();
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETLOADWIDGET_H
