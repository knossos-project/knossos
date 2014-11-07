#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include "annotationwidget.h"
#include "commentswidget.h"
#include "datasetloadwidget.h"
#include "datasetoptionswidget.h"
#include "datasavingwidget.h"
#include "documentationwidget.h"
#include "navigationwidget.h"
#include "splashscreenwidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "tracingtimewidget.h"
#include "viewportsettingswidget.h"
#include "pythonpropertywidget.h"

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : annotationWidgetObject(parent), commentsWidgetObject(parent), datasetLoadWidgetObject(parent)
        , datasetOptionsWidgetObject(parent), dataSavingWidgetObject(parent), docWidgetObject(parent)
        , navigationWidgetObject(parent), pythonPropertyWidgetObject(parent), splashWidgetObject(parent), taskLoginWidgetObject(parent)
        , taskManagementWidgetObject(&taskLoginWidgetObject, parent), tracingTimeWidgetObject(parent)
        , viewportSettingsWidgetObject(parent)

        , annotationWidget(&annotationWidgetObject), commentsWidget(&commentsWidgetObject)
        , datasetLoadWidget(&datasetLoadWidgetObject), datasetOptionsWidget(&datasetOptionsWidgetObject)
        , dataSavingWidget(&dataSavingWidgetObject), docWidget(&docWidgetObject), navigationWidget(&navigationWidgetObject)
        , pythonPropertyWidget(&pythonPropertyWidgetObject), splashWidget(&splashWidgetObject), taskLoginWidget(&taskLoginWidgetObject), taskManagementWidget(&taskManagementWidgetObject)
        , tracingTimeWidget(&tracingTimeWidgetObject), viewportSettingsWidget(&viewportSettingsWidgetObject)
    {
        taskLoginWidgetObject.setTaskManagementWidget(&taskManagementWidgetObject);
        QObject::connect(this->datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults
        , &this->datasetOptionsWidgetObject, &DatasetOptionsWidget::zoomDefaultsClicked);
    }

    AnnotationWidget annotationWidgetObject;
    CommentsWidget commentsWidgetObject;
    DatasetLoadWidget datasetLoadWidgetObject;
    DatasetOptionsWidget datasetOptionsWidgetObject;
    DataSavingWidget dataSavingWidgetObject;
    DocumentationWidget docWidgetObject;
    NavigationWidget navigationWidgetObject;
    PythonPropertyWidget pythonPropertyWidgetObject;
    SplashScreenWidget splashWidgetObject;
    TaskLoginWidget taskLoginWidgetObject;
    TaskManagementWidget taskManagementWidgetObject;
    TracingTimeWidget tracingTimeWidgetObject;
    ViewportSettingsWidget viewportSettingsWidgetObject;

    //FIXME these pointers just point to the objects above
    //one may replace all -> with . in the project and remove these
    AnnotationWidget * const annotationWidget;
    CommentsWidget * const commentsWidget;
    DatasetLoadWidget * const datasetLoadWidget;
    DatasetOptionsWidget * const datasetOptionsWidget;
    DataSavingWidget * const dataSavingWidget;
    DocumentationWidget * const docWidget;
    NavigationWidget * const navigationWidget;
    PythonPropertyWidget * const pythonPropertyWidget;
    SplashScreenWidget * const splashWidget;
    TaskLoginWidget * const taskLoginWidget;
    TaskManagementWidget * const taskManagementWidget;
    TracingTimeWidget * const tracingTimeWidget;
    ViewportSettingsWidget * const viewportSettingsWidget;
};

#endif // WIDGETCONTAINER_H
