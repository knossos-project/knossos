#ifndef DATASETPROPERTYWIDGET_H
#define DATASETPROPERTYWIDGET_H

#include <QWidget>
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QGroupBox;
class DatasetPropertyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DatasetPropertyWidget(QWidget *parent = 0);


protected:
    QGroupBox *localGroup;
    QString dir;
    QLabel *supercubeSizeLabel;
    QLineEdit *path;
    QComboBox *supercubeSize;
    QPushButton *datasetfileDialog;
    QPushButton *cancelButton;
    QPushButton *processButton;

    /* remote */
    QGroupBox *remoteGroup;
    QLineEdit *username;
    QLineEdit *password;
    QLineEdit *url;
    QPushButton *remoteCancelButton;
    QPushButton *connectButton;
    void resetHashtable();
    void closeEvent(QCloseEvent *event);
signals:
    void clearSkeletonSignal();
    void resetLoaderSignal();
public slots:
    void datasetfileDialogClicked();
    void cancelButtonClicked();
    void processButtonClicked();
    void connectButtonClicked();
};

#endif // DATASETPROPERTYWIDGET_H
