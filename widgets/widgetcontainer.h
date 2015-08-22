#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include "annotationwidget.h"
#include "datasetloadwidget.h"
#include "datasetoptionswidget.h"
#include "datasavingwidget.h"
#include "documentationwidget.h"
#include "navigationwidget.h"
#include "snapshotwidget.h"
#include "splashscreenwidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "viewportsettingswidget.h"
#include "pythonpropertywidget.h"

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : annotationWidgetObject(parent), datasetLoadWidgetObject(parent)
        , datasetOptionsWidgetObject(parent), dataSavingWidgetObject(parent), docWidgetObject(parent)
        , navigationWidgetObject(parent), pythonPropertyWidgetObject(parent), snapshotWidgetObject(parent), splashWidgetObject(parent)
        , taskManagementWidgetObject(parent), viewportSettingsWidgetObject(parent)

        , annotationWidget(&annotationWidgetObject), datasetLoadWidget(&datasetLoadWidgetObject), datasetOptionsWidget(&datasetOptionsWidgetObject)
        , dataSavingWidget(&dataSavingWidgetObject), docWidget(&docWidgetObject), navigationWidget(&navigationWidgetObject)
        , pythonPropertyWidget(&pythonPropertyWidgetObject), snapshotWidget(&snapshotWidgetObject), splashWidget(&splashWidgetObject), taskManagementWidget(&taskManagementWidgetObject)
        , viewportSettingsWidget(&viewportSettingsWidgetObject)
    {
        QObject::connect(datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults, &datasetOptionsWidgetObject, &DatasetOptionsWidget::zoomDefaultsClicked);
        QObject::connect(&viewportSettingsWidget->datasetAndSegmentationTab, &DatasetAndSegmentationOptionsTab::volumeRenderToggled, snapshotWidget, &SnapshotWidget::updateOptionVisibility);
    }

    AnnotationWidget annotationWidgetObject;
    DatasetLoadWidget datasetLoadWidgetObject;
    DatasetOptionsWidget datasetOptionsWidgetObject;
    DataSavingWidget dataSavingWidgetObject;
    DocumentationWidget docWidgetObject;
    NavigationWidget navigationWidgetObject;
    PythonPropertyWidget pythonPropertyWidgetObject;
    SnapshotWidget snapshotWidgetObject;
    SplashScreenWidget splashWidgetObject;
    TaskManagementWidget taskManagementWidgetObject;
    ViewportSettingsWidget viewportSettingsWidgetObject;

    //FIXME these pointers just point to the objects above
    //one may replace all -> with . in the project and remove these
    AnnotationWidget * const annotationWidget;
    DatasetLoadWidget * const datasetLoadWidget;
    DatasetOptionsWidget * const datasetOptionsWidget;
    DataSavingWidget * const dataSavingWidget;
    DocumentationWidget * const docWidget;
    NavigationWidget * const navigationWidget;
    PythonPropertyWidget * const pythonPropertyWidget;
    SnapshotWidget * const snapshotWidget;
    SplashScreenWidget * const splashWidget;
    TaskManagementWidget * const taskManagementWidget;
    ViewportSettingsWidget * const viewportSettingsWidget;

    void hideAll() {
        annotationWidget->hide();
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
        viewportSettingsWidget->hide();
    }
};

#endif // WIDGETCONTAINER_H
