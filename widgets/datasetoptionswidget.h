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

class QDoubleSpinBox;
class QCheckBox;


#define LOCK_DATASET_ORIENTATION_DEFAULT (false)

class DatasetOptionsWidget : public QDialog {
    friend class MainWindow;
    Q_OBJECT
public:
    explicit DatasetOptionsWidget(QWidget *parent = 0);
public slots:
    void zoomDefaultsClicked();
    void lockDatasetMagChecked(bool on);
    void orthogonalSpinBoxChanged(double value);
    void skeletonSpinBoxChanged(double value);
    void update();
    void loadSettings();
    void saveSettings();
    void updateCompressionRatioDisplay();
protected:
    /*! Necessary helper variables to enable relative (instead of constant) incrementation/decrementation of zoom spin boxes.
     * Zoom steps become smaller with higher zoom levels and vice versa (smoother impression to the user).
     * See the spinBoxChanged slots for more details. */
    float lastZoomSkel;
    bool userZoomSkel;

    // compression section
    QLabel compressionLabel;
    // zoom section
    QLabel zoomSectionLabel{"Zoom Settings"};
    QLabel *orthogonalDataViewportLabel;
    QLabel *skeletonViewportLabel;
    QDoubleSpinBox *orthogonalDataViewportSpinBox;
    QDoubleSpinBox *skeletonViewportSpinBox;
    QPushButton *zoomDefaultsButton;
    // multires section
    QLabel multiresSectionLabel{"Magnification Settings"};
    QLabel *lockDatasetLabel;
    QCheckBox *lockDatasetCheckBox;
    QLabel *currentActiveMagDatasetLabel;
    QLabel *highestActiveMagDatasetLabel;
    QLabel *lowestActiveMagDatasetLabel;
signals:
    void visibilityChanged(bool);
private:
    void showEvent(QShowEvent *) override {
        emit visibilityChanged(true);
    }
    void hideEvent(QHideEvent *) override {
        emit visibilityChanged(false);
    }
};

#endif // DATASETOPTIONSWIDGET_H
