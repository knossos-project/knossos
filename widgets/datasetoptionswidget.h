#ifndef DATASETOPTIONSWIDGET_H
#define DATASETOPTIONSWIDGET_H



/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>

class QCheckBox;


#define LOCK_DATASET_ORIENTATION_DEFAULT (false)

// a horizontal slider with ticks and mag labels at each mag switch position
class ZoomSlider : public QSlider {
    Q_OBJECT
    using QSlider::setTickPosition; // prevent default tick painting in parent.
protected:
    virtual void paintEvent(QPaintEvent *ev) override;
public:
    int highestMag{0};
    int numTicks{0};
};

class DatasetOptionsWidget : public QDialog {
    friend class MainWindow;
    Q_OBJECT
    /*! Necessary helper variables to enable relative (instead of constant) incrementation/decrementation of zoom spin boxes.
     * Zoom steps become smaller with higher zoom levels and vice versa (smoother impression to the user).
     * See the spinBoxChanged slots for more details. */
    float lastZoomSkel;
    bool userZoomSkel;

    // compression section
    QLabel compressionLabel;
    // zoom section
    QLabel zoomSectionLabel{"Zoom Settings"};
    QLabel orthogonalDataViewportLabel{"Orthogonal Viewports"};
    QLabel *skeletonViewportLabel;
    QDoubleSpinBox *skeletonViewportSpinBox;
    QDoubleSpinBox orthoZoomSpinBox;
    ZoomSlider orthoZoomSlider;
    QPushButton *zoomDefaultsButton;
    // multires section
    QLabel multiresSectionLabel{"Magnification Settings"};
    QLabel *lockDatasetLabel;
    QCheckBox *lockDatasetCheckBox;
    QLabel *currentActiveMagDatasetLabel;
    QLabel *highestActiveMagDatasetLabel;
    QLabel *lowestActiveMagDatasetLabel;
    void applyZoom(const float newScreenPxXPerDataPx);
    void reinitializeOrthoZoomWidgets();
    float zoomStep{1};
    void updateOrthogonalZoomSpinBox();
    void updateOrthogonalZoomSlider();

    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *event) override {
        QDialog::hideEvent(event);
        emit visibilityChanged(false);
    }
public:
    explicit DatasetOptionsWidget(QWidget *, class DatasetLoadWidget * datasetLoadWidget);
public slots:
    void zoomDefaultsClicked();
    void skeletonSpinBoxChanged(double value);
    void update();
    void loadSettings();
    void saveSettings();
    void updateCompressionRatioDisplay();

signals:
    void visibilityChanged(bool);
};

#endif // DATASETOPTIONSWIDGET_H
