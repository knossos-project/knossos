#ifndef DATASETPROPERTYWIDGET_H
#define DATASETPROPERTYWIDGET_H

#include <QDialog>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class DatasetPropertyWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DatasetPropertyWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();
    void changeDataSet(bool isGUI);

protected:
    QGroupBox *localGroup;
    QComboBox *path;
    QPushButton *datasetfileDialog;
    QSpinBox *supercubeEdgeSpin;
    QPushButton *cancelButton;
    QPushButton *processButton;
    void closeEvent(QCloseEvent *event);
    void waitForLoader();
    QStringList getRecentDirsItems();
signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void userMoveSignal(int x, int y, int z, int serverMovement);
    void datasetSwitchZoomDefaults();
    void startLoaderSignal();
public slots:
    void datasetfileDialogClicked();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETPROPERTYWIDGET_H
