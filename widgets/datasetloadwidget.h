#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include "knossos-global.h"
#include "coordinate.h"

#include <QCheckBox>
#include <QDialog>
#include <QTextDocument>
#include <QListWidget>
#include <QTableWidget>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextDocument;
class QRadioBox;
class QListWidget;
class QTableWidget;
class QScrollArea;

class DatasetLoadWidget : public QDialog {
    Q_OBJECT
    QLabel cubeEdgeLabel{"Cubesize"};
    QSpinBox cubeEdgeSpin;
public:
    explicit DatasetLoadWidget(QWidget *parent = 0);
    void changeDataset(bool isGUI);
    bool loadDataset(bool isGUI, bool loadOverlay = false, QString path = "");
    void saveSettings();
    void loadSettings();
    void applyGeometrySettings();
    void processTableWidgetClicked();
    void addRow(int row, int col);
    void addClicked();
    void delClicked();
    struct Datasetinfo{
        Coordinate boundary;
        floatCoordinate scale{0,0,0};
        int magnification = 0, cubeEdgeLength = 0, compressionRatio = 0;
        bool remote = false;
        std::string experimentname{""},ftphostname{""}, ftpbasepath{""};
    };
    Datasetinfo datasetinfo;
    Datasetinfo getConfigFileInfo(const char *path);
    QCheckBox segmentationOverlayCheckbox{"load segmentation overlay"};
    QStringList getRecentPathItems();
    QSpinBox *supercubeEdgeSpin;
    QLabel *infolabel;
    QScrollArea *scrollarea;
    QString datasetPath;
    QTableWidget *tableWidget;
protected:
    QLabel *supercubeSizeLabel;
    QPushButton *cancelButton;
    QPushButton *processButton;
    void MessageBoxInformation(QString message);

signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetCompression();
    void datasetChanged(Coordinate min, Coordinate max, bool showOverlays);
    void userMoveSignal(int x, int y, int z, UserMoveType userMoveType, ViewportType viewportType);
    void datasetSwitchZoomDefaults();
    void startLoaderSignal();
    void breakLoaderSignal();
public slots:
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETLOADWIDGET_H
