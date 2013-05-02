#ifndef ZOOMANDMULTIRESWIDGET_H
#define ZOOMANDMULTIRESWIDGET_H

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

class QSlider;
class QLabel;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class ZoomAndMultiresWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ZoomAndMultiresWidget(QWidget *parent = 0);
        
public slots:
    void zoomDefaultsSlot();
    void lockDatasetMagSlot(bool on);
    void orthogonalSliderSlot(int value);
    void skeletonSliderSlot(int value);
    void orthogonalSpinBoxSlot(double value);
    void skeletonSpinBoxSlot(double value);
    void update();
protected:
    void closeEvent(QCloseEvent *event);
public:
    // top layout
    QLabel *orthogonalDataViewportLabel;
    QLabel *skeletonViewLabel;
    QSlider *orthogonalDataViewportSlider;
    QSlider *skeletonViewSlider;
    QDoubleSpinBox *orthogonalDataViewportSpinBox;
    QDoubleSpinBox *skeletonViewSpinBox;

    QPushButton *zoomDefaultsButton;

    // bottom layout
    QLabel *lockDatasetLabel;
    QCheckBox *lockDatasetCheckBox;
    QLabel *currentActiveMagDatasetLabel;
    QLabel *highestActiveMagDatasetLabel;
    QLabel *lowestActiveMagDatasetLabel;
signals:
    void refreshSignal();
    void uncheckSignal();
};

#endif // ZOOMANDMULTIRESWIDGET_H
