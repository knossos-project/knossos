#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include "annotationwidget.h"
#include "appearancewidget.h"
#include "datasetloadwidget.h"
#include "datasetoptionswidget.h"
#include "datasavingwidget.h"
#include "documentationwidget.h"
#include "navigationwidget.h"
#include "snapshotwidget.h"
#include "splashscreenwidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "pythonpropertywidget.h"

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : annotationWidgetObject(parent), appearanceWidgetObject(parent), datasetLoadWidgetObject(parent)
        , datasetOptionsWidgetObject(parent), dataSavingWidgetObject(parent), docWidgetObject(parent)
        , navigationWidgetObject(parent), pythonPropertyWidgetObject(parent), snapshotWidgetObject(parent), splashWidgetObject(parent)
        , taskManagementWidgetObject(parent)

        , annotationWidget(&annotationWidgetObject), appearanceWidget(&appearanceWidgetObject), datasetLoadWidget(&datasetLoadWidgetObject), datasetOptionsWidget(&datasetOptionsWidgetObject)
        , dataSavingWidget(&dataSavingWidgetObject), docWidget(&docWidgetObject), navigationWidget(&navigationWidgetObject)
        , pythonPropertyWidget(&pythonPropertyWidgetObject), snapshotWidget(&snapshotWidgetObject), splashWidget(&splashWidgetObject), taskManagementWidget(&taskManagementWidgetObject)
    {
        QObject::connect(datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults, &datasetOptionsWidgetObject, &DatasetOptionsWidget::zoomDefaultsClicked);
        QObject::connect(&appearanceWidget->datasetAndSegmentationTab, &DatasetAndSegmentationTab::volumeRenderToggled, snapshotWidget, &SnapshotWidget::updateOptionVisibility);
    }

    AnnotationWidget annotationWidgetObject;
    AppearanceWidget appearanceWidgetObject;
    DatasetLoadWidget datasetLoadWidgetObject;
    DatasetOptionsWidget datasetOptionsWidgetObject;
    DataSavingWidget dataSavingWidgetObject;
    DocumentationWidget docWidgetObject;
    NavigationWidget navigationWidgetObject;
    PythonPropertyWidget pythonPropertyWidgetObject;
    SnapshotWidget snapshotWidgetObject;
    SplashScreenWidget splashWidgetObject;
    TaskManagementWidget taskManagementWidgetObject;

    //FIXME these pointers just point to the objects above
    //one may replace all -> with . in the project and remove these
    AnnotationWidget * const annotationWidget;
    AppearanceWidget * const appearanceWidget;
    DatasetLoadWidget * const datasetLoadWidget;
    DatasetOptionsWidget * const datasetOptionsWidget;
    DataSavingWidget * const dataSavingWidget;
    DocumentationWidget * const docWidget;
    NavigationWidget * const navigationWidget;
    PythonPropertyWidget * const pythonPropertyWidget;
    SnapshotWidget * const snapshotWidget;
    SplashScreenWidget * const splashWidget;
    TaskManagementWidget * const taskManagementWidget;

    void hideAll() {
        annotationWidget->hide();
        appearanceWidget->hide();
        datasetLoadWidget->hide();
        datasetOptionsWidget->hide();
        dataSavingWidget->hide();
        docWidget->hide();
        navigationWidget->hide();
        pythonPropertyWidget->hide();
        pythonPropertyWidget->closeTerminal();
        snapshotWidget->hide();
        splashWidget->hide();
        taskManagementWidget->hide();
    }
};

#endif // WIDGETCONTAINER_H
