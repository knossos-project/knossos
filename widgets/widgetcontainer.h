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
#include "pythoninterpreterwidget.h"
#include "pythonpropertywidget.h"

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : annotationWidget(parent), appearanceWidget(parent), datasetLoadWidget(parent)
        , datasetOptionsWidget(parent, &datasetLoadWidget), dataSavingWidget(parent), docWidget(parent)
        , navigationWidget(parent), pythonPropertyWidget(parent), pythonInterpreterWidget(parent)
        , snapshotWidget(parent), splashWidget(parent), taskManagementWidget(parent)
    {
        QObject::connect(&datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults, &datasetOptionsWidget, &DatasetOptionsWidget::zoomDefaultsClicked);
        QObject::connect(&appearanceWidget.datasetAndSegmentationTab, &DatasetAndSegmentationTab::volumeRenderToggled, &snapshotWidget, &SnapshotWidget::updateOptionVisibility);
    }

    AnnotationWidget annotationWidget;
    AppearanceWidget appearanceWidget;
    DatasetLoadWidget datasetLoadWidget;
    DatasetOptionsWidget datasetOptionsWidget;
    DataSavingWidget dataSavingWidget;
    DocumentationWidget docWidget;
    NavigationWidget navigationWidget;
    PythonPropertyWidget pythonPropertyWidget;
    PythonInterpreterWidget pythonInterpreterWidget;
    SnapshotWidget snapshotWidget;
    SplashScreenWidget splashWidget;
    TaskManagementWidget taskManagementWidget;

    void hideAll() {
        annotationWidget.hide();
        appearanceWidget.hide();
        datasetLoadWidget.hide();
        datasetOptionsWidget.hide();
        dataSavingWidget.hide();
        docWidget.hide();
        navigationWidget.hide();
        pythonPropertyWidget.hide();
        pythonInterpreterWidget.hide();
        snapshotWidget.hide();
        splashWidget.hide();
        taskManagementWidget.hide();
    }
};

#endif // WIDGETCONTAINER_H
