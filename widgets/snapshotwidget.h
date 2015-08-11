#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewport.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class SnapshotWidget : public QDialog
{
    Q_OBJECT
    QString saveDir;
    QComboBox sizeCombo;
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpYZRadio{"YZ viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withAxesCheck{"Dataset Axes"}, withOverlayCheck{"Segmentation overlay"}, withSkeletonCheck{"Skeleton overlay"}, withScaleCheck{"Physical scale"}, withVpPlanes{"Viewport planes"};
    QPushButton snapshotButton{"Take snapshot"};
    QVBoxLayout mainLayout;
    ViewportType getCheckedViewport() const;
    QString defaultFilename() const;
public:
    explicit SnapshotWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();
signals:
    void snapshotRequest(const QString & path, ViewportType vp, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);
};

#endif // SNAPSHOTWIDGET_H
