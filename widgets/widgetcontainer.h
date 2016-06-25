/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

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
        QObject::connect(&datasetLoadWidget, &DatasetLoadWidget::updateDatasetCompression, &datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
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
