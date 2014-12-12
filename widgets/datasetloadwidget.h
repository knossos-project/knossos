#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include "knossos-global.h"

#include <QCheckBox>
#include <QDialog>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QRadioBox;

class DatasetLoadWidget : public QDialog {
    Q_OBJECT
public:
    explicit DatasetLoadWidget(QWidget *parent = 0);
    void changeDataset(bool isGUI);
    void onRadiobutton();
    void onUrlAdd();
    void updateUsername();
    void updatePassword();
    void saveSettings();
    void loadSettings();
    void applyGeometrySettings();
    std::string lastused;
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QComboBox *pathDropdown;
    QRadioButton *local;
    QRadioButton *remote;
    QPushButton *datasetfileDialog;
    QStringList getRecentPathItems();
    QStringList getRecentUsernames();
    QStringList getRecentPasswords();
    QStringList getRecentHosts();
    QSpinBox *supercubeEdgeSpin;
    QComboBox *urlCombo;
    QStringList usernameList;
    QStringList passwordList;
    QLineEdit *usernameField;
    QLineEdit *passwordField;
protected:
    QLabel *supercubeSizeLabel;
    QPushButton *cancelButton;
    QPushButton *processButton;
    QLabel *host;
    QLabel *username;
    QLabel *passwd;

signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetCompression();
    void datasetChanged(Coordinate center, Coordinate range);
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void datasetSwitchZoomDefaults();
    void setlastusedlocal();
    void startLoaderSignal();
    void breakLoaderSignal();
public slots:
    void datasetfileDialogClicked();
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void onUrlChange();
    void processButtonClicked();
};

#endif // DATASETLOADWIDGET_H
