/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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

#include "aboutdialog.h"
#include "annotationwidget.h"
#include "datasetloadwidget.h"
#include "GuiConstants.h"
#include "preferenceswidget.h"
#include "pythoninterpreterwidget.h"
#include "pythonpropertywidget.h"
#include "snapshotwidget.h"
#include "task/taskloginwidget.h"
#include "task/taskmanagementwidget.h"
#include "zoomwidget.h"
#include "layerdialog.h"

#include <QSettings>

struct WidgetContainer {
    WidgetContainer(QWidget * parent)
        : aboutDialog(parent), annotationWidget(parent), datasetLoadWidget(parent)
        , preferencesWidget(parent), pythonInterpreterWidget(parent), pythonPropertyWidget(parent)
        , snapshotWidget(parent), taskManagementWidget(parent), zoomWidget(parent, &datasetLoadWidget)
        , layerDialogWidget(parent)
    {
        QObject::connect(&datasetLoadWidget, &DatasetLoadWidget::datasetSwitchZoomDefaults, &zoomWidget, &ZoomWidget::zoomDefaultsClicked);
        QObject::connect(&preferencesWidget.viewportTab.addArbVPCheckBox, &QCheckBox::toggled, &snapshotWidget.vpArbRadio, &QRadioButton::setVisible);
    }

    AboutDialog aboutDialog;
    AnnotationWidget annotationWidget;
    DatasetLoadWidget datasetLoadWidget;
    PreferencesWidget preferencesWidget;
    PythonInterpreterWidget pythonInterpreterWidget;
    PythonPropertyWidget pythonPropertyWidget;
    SnapshotWidget snapshotWidget;
    TaskManagementWidget taskManagementWidget;
    ZoomWidget zoomWidget;
    LayerDialogWidget layerDialogWidget;

    void applyVisibility() {
        QSettings settings;
        annotationWidget.setVisible(settings.value(ANNOTATION_WIDGET + '/' + VISIBLE, false).toBool());
        preferencesWidget.setVisible(settings.value(PREFERENCES_WIDGET + '/' + VISIBLE, false).toBool());
        pythonInterpreterWidget.setVisible(settings.value(PYTHON_TERMINAL_WIDGET + '/' + VISIBLE, false).toBool());
        pythonPropertyWidget.setVisible(settings.value(PYTHON_PROPERTY_WIDGET + '/' + VISIBLE, false).toBool());
        snapshotWidget.setVisible(settings.value(SNAPSHOT_WIDGET + '/' + VISIBLE, false).toBool());
        zoomWidget.setVisible(settings.value(ZOOM_WIDGET + '/' + VISIBLE, false).toBool());
        layerDialogWidget.setVisible(settings.value(LAYER_DIALOG_WIDGET + '/' + VISIBLE, false).toBool());
    }

    void hideAll() {
        aboutDialog.hide();
        annotationWidget.hide();
        datasetLoadWidget.hide();
        preferencesWidget.hide();
        pythonPropertyWidget.hide();
        pythonInterpreterWidget.hide();
        snapshotWidget.hide();
        taskManagementWidget.hide();
        zoomWidget.hide();
        layerDialogWidget.hide();
    }
};

#endif // WIDGETCONTAINER_H
