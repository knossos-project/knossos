#ifndef DATASETLOADWIDGET_H
#define DATASETLOADWIDGET_H

#include "coordinate.h"
#include "loader.h"
#include "widgets/viewport.h"

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
    void gatherHeidelbrainDatasetInformation(QString &);
    bool loadDataset(QString path = "", const bool keepAnnotation = false);
    void saveSettings();
    void loadSettings();
    void applyGeometrySettings();
    void updateDatasetInfo();
    void insertDatasetRow(const QString & dataset, const int pos);
    void datasetCellChanged(int row, int col);
    bool parseGoogleJson(const QString & json_raw);
    bool parseWebKnossosJson(const QString & json_raw);
    QString extractWebKnossosToken(QString & json_raw);
    struct Datasetinfo{
        Coordinate boundary;
        floatCoordinate scale{0,0,0};
        int magnification = 0, cubeEdgeLength = 0, compressionRatio = 0;
        bool remote = false;
        std::string experimentname{""},ftphostname{""}, ftpbasepath{""};
    };
    Datasetinfo datasetinfo;
    Datasetinfo getConfigFileInfo(const QString &path);
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

signals:
    void clearSkeletonSignalGUI();
    void clearSkeletonSignalNoGUI();
    void changeDatasetMagSignal(uint upOrDownFlag);
    void updateDatasetCompression();
    void datasetChanged(bool showOverlays);
    void datasetSwitchZoomDefaults();
public slots:
    void adaptMemoryConsumption();
    void cancelButtonClicked();
    void processButtonClicked();
};

#endif // DATASETLOADWIDGET_H
