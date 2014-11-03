#ifndef DATASETREMOTEWIDGET_H
#define DATASETREMOTEWIDGET_H

#include "knossos-global.h"

#include <QCheckBox>
#include <QWidget>

class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class DatasetRemoteWidget : public QWidget {
    Q_OBJECT
public:
    explicit DatasetRemoteWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();
    void changeDataset(bool isGUI);
    void cancelButtonClicked();
    void processButtonClicked();
    void waitForLoader();
    QLineEdit *urlField;
    QLineEdit *usernameField;
    QLineEdit *passwordField;
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QSpinBox *supercubeEdgeSpin;

protected:
    QGroupBox *localGroup;
    QLabel *supercubeSizeLabel;
    QLabel *host;
    QLabel *username;
    QLabel *passwd;
    QPushButton *cancelButton;
    QPushButton *processButton;

signals:
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void updateDatasetCompression();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void startLoaderSignal();
    void datasetSwitchZoomDefaults();
};

#endif // DATASETREMOTEWIDGET_H
