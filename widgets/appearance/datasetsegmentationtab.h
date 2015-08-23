#ifndef DATASETANDSEGMENTATIONTAB_H
#define DATASETANDSEGMENTATIONTAB_H

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

class DatasetAndSegmentationTab : public QWidget
{
    friend class AppearanceWidget;
    friend class EventModel;
    Q_OBJECT
    // dataset
    QLabel datasetHeader{"<strong>Dataset</strong>"};
    QFrame datasetSeparator;
    QCheckBox datasetLinearFilteringCheckBox{"Enable linear filtering"};
    QCheckBox useOwnDatasetColorsCheckBox{"Use own dataset colors"};
    QPushButton useOwnDatasetColorsButton{"Load …"};
    QString lutFilePath;
    QLabel datasetDynamicRangeLabel{"Dataset dynamic range"}, biasLabel{"Bias"}, rangeDeltaLabel{"Range delta"};
    QSpinBox biasSpinBox, rangeDeltaSpinBox;
    QSlider biasSlider{Qt::Horizontal}, rangeDeltaSlider{Qt::Horizontal};
    // segmentation overlay
    QLabel segmentationHeader{"<strong>Segmentation</strong>"};
    QFrame segmentationSeparator;
    QLabel segmentationOverlayLabel{"Overlay opacity"};
    QSpinBox segmentationOverlaySpinBox;
    QSlider segmentationOverlaySlider{Qt::Horizontal};
    // segmentation volume
    QCheckBox VolumeRenderFlagCheckBox{"Show volume in 3D viewport"};
    QLabel VolumeOpaquenessLabel{"Volume opacity"};
    QSpinBox VolumeOpaquenessSpinBox;
    QSlider VolumeOpaquenessSlider{Qt::Horizontal};
    QLabel VolumeColorLabel{"Volume background color"};
    QPushButton VolumeColorBox;

    QGridLayout mainLayout;
    void useOwnDatasetColorsButtonClicked(QString path = "");
public:
    explicit DatasetAndSegmentationTab(QWidget *parent = 0);

signals:
    void volumeRenderToggled();
public slots:
};

#endif // DATASETANDSEGMENTATIONTAB_H
