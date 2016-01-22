#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include "coordinate.h"
#include "loader.h"
#include "widgets/viewport.h"

#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTextDocument>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

class QScrollArea;
class QPushButton;

class DatasetLoadWidget : public QDialog {
    Q_OBJECT

    QVBoxLayout mainLayout;
    QTableWidget tableWidget;
    QLabel infoLabel;
    QFrame line;
    QHBoxLayout superCubeEdgeHLayout;
    QSpinBox superCubeEdgeSpin;
    QLabel superCubeSizeLabel;
    QHBoxLayout cubeEdgeHLayout;
    QLabel cubeEdgeLabel{"Cubesize"};
    QSpinBox cubeEdgeSpin;
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QHBoxLayout buttonHLayout;
    QPushButton processButton{"Load Dataset"};
    QPushButton cancelButton{"Close"};
public:
    QUrl datasetUrl;//meh

    explicit DatasetLoadWidget(QWidget *parent = 0);
    void changeDataset(bool isGUI);
    bool loadDataset(const boost::optional<bool> loadOverlay, QString path = "", const bool keepAnnotation = false);
    void saveSettings();
    void loadSettings();
    void applyGeometrySettings();
    void updateDatasetInfo();
    void insertDatasetRow(const QString & dataset, const int pos);
    void datasetCellChanged(int row, int col);
    QString extractWebKnossosToken(QString & json_raw);
    QStringList getRecentPathItems();

signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void updateDatasetCompression();
    void datasetChanged(bool showOverlays);
    void datasetSwitchZoomDefaults();
public slots:
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETLOADWIDGET_H
