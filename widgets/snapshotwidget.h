#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewport.h"

#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class SnapshotWidget : public QDialog
{
    Q_OBJECT
    QString saveDir;
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpYZRadio{"YZ viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withOverlayCheck{"Segmentation overlay"}, withScaleCheck{"Physical scale"};
    QPushButton snapshotButton{"Take snapshot"};
    QVBoxLayout mainLayout;
    ViewportType getCheckedViewport() const;
    QString defaultFilename() const;
public:
    explicit SnapshotWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();
signals:
    void snapshotRequest(const QString path, const ViewportType vp, const bool withOverlay, const bool withScale);
};

#endif // SNAPSHOTWIDGET_H
