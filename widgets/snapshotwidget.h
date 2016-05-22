#ifndef SNAPSHOTWIDGET_H
#define SNAPSHOTWIDGET_H

#include "viewport.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

class SnapshotWidget : public QDialog {
    Q_OBJECT
    QString saveDir;
    QComboBox sizeCombo;
    QRadioButton vpXYRadio{"XY viewport"}, vpXZRadio{"XZ viewport"}, vpZYRadio{"ZY viewport"}, vpArbRadio{"Arb viewport"}, vp3dRadio{"3D viewport"};
    QCheckBox withAxesCheck{"Dataset Axes"}, withOverlayCheck{"Segmentation overlay"}, withSkeletonCheck{"Skeleton overlay"}, withScaleCheck{"Physical scale"}, withVpPlanes{"Viewport planes"};
    QPushButton snapshotButton{"Take snapshot"};
    QVBoxLayout mainLayout;
    uint getCheckedViewport() const;
    QString defaultFilename() const;
public:
    explicit SnapshotWidget(QWidget *parent = 0);
    void saveSettings();
    void loadSettings();
public slots:
    void openForVP(const ViewportType type);
    void updateOptionVisibility();
signals:
    void visibilityChanged(bool);
    void snapshotRequest(const QString & path, const ViewportType vpType, const int size, const bool withAxes, const bool withOverlay, const bool withSkeleton, const bool withScale, const bool withVpPlanes);

private:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
};

#endif // SNAPSHOTWIDGET_H
