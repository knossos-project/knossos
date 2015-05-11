#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewport.h"

#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QVBoxLayout>

class SnapshotWidget : public QDialog
{
    Q_OBJECT
    QLineEdit pathEdit{""};
    QPushButton browsePathButton{"…"};
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpYZRadio{"YZ viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withOverlayCheck{"Segmentation overlay"}, withScaleCheck{"Physical scale"};
    QPushButton snapshotButton{"Take snapshot"};
    QVBoxLayout mainLayout;
public:
    explicit SnapshotWidget(QWidget *parent = 0);

signals:
    void snapshotRequest(const QString path, const ViewportType vp, const bool withOverlay, const bool withScale);
};

#endif // SNAPSHOTWIDGET_H
