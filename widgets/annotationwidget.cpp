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

#include "annotationwidget.h"

#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QSplitter>

AnnotationWidget::AnnotationWidget(QWidget *parent) : DialogVisibilityNotify(parent) {
    setWindowIcon(QIcon(":/resources/icons/graph.png"));
    setWindowTitle("Annotation");

    tabs.addTab(&skeletonTab, "Skeleton");
    tabs.addTab(&segmentationTab, "Segmentation");
    tabs.addTab(&commentsTab, "Comments");

    mainLayout.addWidget(&tabs);
    mainLayout.setContentsMargins({});
    setLayout(&mainLayout);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void AnnotationWidget::setSegmentationVisibility(const bool visible) {
    const auto index = tabs.indexOf(&segmentationTab);
    tabs.setTabEnabled(index, visible);
    const QString tooltip = "Enable the segmentation overlay when loading a dataset";
    tabs.setTabToolTip(index, visible ? "" : tooltip);
}

void AnnotationWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(ANNOTATION_WIDGET);

    width = (settings.value(WIDTH).isNull())? 700 : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().bottomRight().x() - width - 20;
        y = QApplication::desktop()->screen()->rect().bottomRight().y() - height - 50;
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();

    skeletonTab.treeCommentFilter.setText(settings.value(SEARCH_FOR_TREE, "").toString());
    skeletonTab.nodeCommentFilter.setText(settings.value(SEARCH_FOR_NODE, "").toString());

    settings.endGroup();

    if(visible) {
        show();
    }
    else {
        hide();
    }
    setGeometry(x, y, width, height);

    commentsTab.loadSettings();
}

void AnnotationWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(ANNOTATION_WIDGET);

    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(SEARCH_FOR_TREE, skeletonTab.treeCommentFilter.text());
    settings.setValue(SEARCH_FOR_NODE, skeletonTab.nodeCommentFilter.text());
    settings.endGroup();

    commentsTab.saveSettings();
}
