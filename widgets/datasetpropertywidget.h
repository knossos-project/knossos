#ifndef DATASETPROPERTYWIDGET_H
#define DATASETPROPERTYWIDGET_H

#include <QDialog>
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QGroupBox;
class DatasetPropertyWidget : public QDialog
{
    Q_OBJECT
public:
    explicit DatasetPropertyWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();

protected:
    QGroupBox *localGroup;
    QComboBox *path;
    QPushButton *datasetfileDialog;
    QPushButton *cancelButton;
    QPushButton *processButton;
    void closeEvent(QCloseEvent *event);
    void waitForLoader();
    QStringList getRecentDirsItems();
signals:
    void clearSkeletonSignal();
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
