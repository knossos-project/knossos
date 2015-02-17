#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include "knossos-global.h"
#include "coordinate.h"

#include <QCheckBox>
#include <QDialog>
#include <QTextDocument>
#include <QListWidget>

class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextDocument;
class QRadioBox;
class QListWidget;
class QScrollArea;

class DatasetLoadWidget : public QDialog {
    Q_OBJECT
    QLabel cubeEdgeLabel{"Cubesize"};
    QSpinBox cubeEdgeSpin;
public:
    explicit DatasetLoadWidget(QWidget *parent = 0);
    void changeDataset(bool isGUI);
    bool loadDataset(bool isGUI, QString path = "");
    void saveSettings();
    void loadSettings();
    void applyGeometrySettings();
    void processListWidgetClicked();
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
    QLineEdit *pathLineEdit;
    QPushButton *datasetfileDialog;
    QPushButton *fileDialogButton;
    QPushButton *deleteButton;
    QStringList getRecentPathItems();
    QSpinBox *supercubeEdgeSpin;
    QLabel *infolabel;
    QListWidget *datasetlistwidget;
    QScrollArea *scrollarea;
    QString datasetPath;
protected:
    QLabel *supercubeSizeLabel;
    QPushButton *cancelButton;
    QPushButton *processButton;

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
    void datasetfileDialogClicked();
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
    void addDatasetClicked();
    void deleteDataset();
};

#endif // DATASETLOADWIDGET_H
