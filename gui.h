#ifndef GUI_H
#define GUI_H

#include <QObject>

class GUI : public QObject
{
    Q_OBJECT
public:
    explicit GUI(QObject *parent = 0);

    static bool addRecentFile(char *path, uint32_t pos);
    static void UI_saveSkeleton(int32_t increment);
    static bool cpBaseDirectory(char *target, char *path, size_t len);

    static void yesNoPrompt(void *par, char *promptString, void (*yesCb)(), void (*noCb)());
    static void treeColorAdjustmentsChanged();

    // from knossos-global.h

    void quitKnossos();
    int32_t initGUI();
    void updateAGconfig();

    void UI_workModeAdd();
    void UI_workModeLink();
    void UI_workModeDrop();
    //void saveSkelCallback(AG_Event *event);

    void UI_saveSettings();
    //void UI_loadSkeleton(AG_Event *event);
    void UI_pasteClipboardCoordinates();
    void UI_copyClipboardCoordinates();
    void UI_zoomOrthogonals(float step);
    void createToolsWin();
    void createViewPortPrefWin();
    static void reloadDataSizeWin();

     void createMenuBar();
     void createCoordBarWin();
     void createSkeletonVpToolsWin();
     void createDataSizeWin();
     void createNavWin();
     void createConsoleWin();
     void createAboutWin();

     void createNavOptionsWin();
     void createDisplayOptionsWin();
     void createSaveOptionsWin();
     void createSyncOptionsWin();
     void createRenderingOptionsWin();
     void createVolTracingOptionsWin();
     void createSpatialLockingOptionsWin();

     void createDataSetStatsWin();
     void createZoomingWin();
     void createTracingTimeWin();
     void createCommentsWin();
     void createLoadDatasetImgJTableWin();
     void createLoadTreeImgJTableWin();
     void createSetDynRangeWin();

signals:
    
public slots:
    
};

#endif // GUI_H
