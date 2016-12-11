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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include "annotationwidget.h"
#include "preferenceswidget.h"
#include "datasetloadwidget.h"
#include "datasetoptionswidget.h"
#include "GuiConstants.h"
#include "snapshotwidget.h"
#include "splashscreenwidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "pythoninterpreterwidget.h"
#include "pythonpropertywidget.h"

#include <QSettings>

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : annotationWidget(parent), preferencesWidget(parent)
        , datasetLoadWidget(parent), datasetOptionsWidget(parent, &datasetLoadWidget)
        , pythonInterpreterWidget(parent), pythonPropertyWidget(parent)
        , snapshotWidget(parent), splashWidget(parent), taskManagementWidget(parent)
    {
        QObject::connect(&datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults, &datasetOptionsWidget, &DatasetOptionsWidget::zoomDefaultsClicked);
        QObject::connect(&datasetLoadWidget, &DatasetLoadWidget::updateDatasetCompression, &datasetOptionsWidget, &DatasetOptionsWidget::updateCompressionRatioDisplay);
        QObject::connect(&preferencesWidget.datasetAndSegmentationTab, &DatasetAndSegmentationTab::volumeRenderToggled, &snapshotWidget, &SnapshotWidget::updateOptionVisibility);
    }

    AnnotationWidget annotationWidget;
    PreferencesWidget preferencesWidget;
    DatasetLoadWidget datasetLoadWidget;
    DatasetOptionsWidget datasetOptionsWidget;
    PythonInterpreterWidget pythonInterpreterWidget;
    PythonPropertyWidget pythonPropertyWidget;
    SnapshotWidget snapshotWidget;
    SplashScreenWidget splashWidget;
    TaskManagementWidget taskManagementWidget;

    void applyVisibility() {
        QSettings settings;
        annotationWidget.setVisible(settings.value(ANNOTATION_WIDGET + '/' + VISIBLE, false).toBool());
        preferencesWidget.setVisible(settings.value(PREFERENCES_WIDGET + '/' + VISIBLE, false).toBool());
        datasetOptionsWidget.setVisible(settings.value(DATASET_OPTIONS_WIDGET + '/' + VISIBLE, false).toBool());
        pythonInterpreterWidget.setVisible(settings.value(PYTHON_TERMINAL_WIDGET + '/' + VISIBLE, false).toBool());
        pythonPropertyWidget.setVisible(settings.value(PYTHON_PROPERTY_WIDGET + '/' + VISIBLE, false).toBool());
        snapshotWidget.setVisible(settings.value(SNAPSHOT_WIDGET + '/' + VISIBLE, false).toBool());
    }

    void hideAll() {
        annotationWidget.hide();
        preferencesWidget.hide();
        datasetLoadWidget.hide();
        datasetOptionsWidget.hide();
        pythonPropertyWidget.hide();
        pythonInterpreterWidget.hide();
        snapshotWidget.hide();
        splashWidget.hide();
        taskManagementWidget.hide();
    }
};

#endif // WIDGETCONTAINER_H
