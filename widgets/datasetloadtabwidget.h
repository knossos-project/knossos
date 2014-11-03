#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include <QCheckBox>
#include <QDialog>

#include "datasetload/datasetlocalwidget.h"
#include "datasetload/datasetremotewidget.h"

class QTabWidget;

class DatasetLoadTabWidget : public QDialog {
    Q_OBJECT
public:
    DatasetLocalWidget * datasetLocalWidget;
    DatasetRemoteWidget * datasetRemoteWidget;
    explicit DatasetLoadTabWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();

private:
    QTabWidget *tabWidget;

};

#endif // DATASETLOADWIDGET_H

