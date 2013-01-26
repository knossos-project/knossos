#include "zoomandmultireswidget.h"

extern struct stateInfo *state;

/**
  * In GUI.c some labels are changed:
  * highest Available Mag = lowest Avaible Mag and otherwise. Why ??
  *
  * In zoomDefaults the vpConfigs are reset to ZOOM_MIN and ZOOM_MAX
  * but if the spinboxes are changed it is state->viewerState->zoomOrthoVps, why ??
  */


ZoomAndMultiresWidget::ZoomAndMultiresWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Zoom and Multiresolution Settings");
    // top layout
    QGridLayout *topLayout = new QGridLayout();
    this->orthogonalDataViewportLabel = new QLabel("Orthogonal Data Viewport");
    this->skeletonViewLabel = new QLabel("Skeleton View");

    this->orthogonalDataViewportSlider = new QSlider(Qt::Horizontal);
    this->skeletonViewSlider = new QSlider(Qt::Horizontal);

    this->orthogonalDataViewportSpinBox = new QDoubleSpinBox();
    this->orthogonalDataViewportSpinBox->setMaximum(VPZOOMMAX);
    this->orthogonalDataViewportSpinBox->setMinimum(VPZOOMMIN);
    this->orthogonalDataViewportSpinBox->setSingleStep(0.01);

    this->skeletonViewSpinBox = new QDoubleSpinBox();
    this->skeletonViewSpinBox->setMaximum(0.5);
    this->skeletonViewSpinBox->setMinimum(0.0);
    this->skeletonViewSpinBox->setSingleStep(0.01);

    topLayout->addWidget(this->orthogonalDataViewportLabel, 0, 1);
    topLayout->addWidget(this->orthogonalDataViewportSlider, 0, 2);
    topLayout->addWidget(this->orthogonalDataViewportSpinBox, 0, 3);

    topLayout->addWidget(this->skeletonViewLabel, 1, 1);
    topLayout->addWidget(this->skeletonViewSlider, 1, 2);
    topLayout->addWidget(this->skeletonViewSpinBox, 1, 3);

    this->zoomDefaultsButton = new QPushButton("All Zoom defaults");
    topLayout->addWidget(zoomDefaultsButton, 2, 1);

    // main layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(topLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);
    mainLayout->addSpacing(10);

    this->lockDatasetLabel = new QLabel("Lock dataset to current mag");
    this->lockDatasetCheckBox = new QCheckBox();

    QString currentActiveMag = QString("Currently active mag dataset: %1").arg(state->magnification);
    QString highestActiveMag = QString("Highest active mag dataset: %1").arg(state->highestAvailableMag);
    QString lowestActiveMag = QString("Lowest active mag dataset: %1").arg(state->lowestAvailableMag);

    this->currentActiveMagDatasetLabel = new QLabel(currentActiveMag);
    this->highestActiveMagDatasetLabel = new QLabel(highestActiveMag);
    this->lowestActiveMagDatasetLabel = new QLabel(lowestActiveMag);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(lockDatasetLabel, 0, 1);
    gridLayout->addWidget(lockDatasetCheckBox, 0, 2);

    mainLayout->addLayout(gridLayout);

    mainLayout->addWidget(currentActiveMagDatasetLabel);
    mainLayout->addWidget(highestActiveMagDatasetLabel);
    mainLayout->addWidget(lowestActiveMagDatasetLabel);


    this->setLayout(mainLayout);

    connect(orthogonalDataViewportSlider, SIGNAL(valueChanged(int)), this, SLOT(orthogonalSliderSlot(int)));
    connect(orthogonalDataViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(orthogonalSpinBoxSlot(double)));

    connect(this->skeletonViewSlider, SIGNAL(valueChanged(int)), this, SLOT(skeletonSliderSlot(int)));
    connect(this->skeletonViewSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxSlot(double)));

    connect(this->zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsSlot()));
    connect(this->lockDatasetCheckBox, SIGNAL(toggled(bool)), this, SLOT(lockDatasetMagSlot(bool)));
}

void ZoomAndMultiresWidget::orthogonalSliderSlot(int value) {
    float result = value / 100.0;
    this->orthogonalDataViewportSpinBox->setValue(result);
}

void ZoomAndMultiresWidget::orthogonalSpinBoxSlot(double value) {

}

void ZoomAndMultiresWidget::skeletonSliderSlot(int value) {
    float result = value / 100.0;
    this->skeletonViewSpinBox->setValue(result);

}

void ZoomAndMultiresWidget::skeletonSpinBoxSlot(double value) {

}

void ZoomAndMultiresWidget::lockDatasetMagSlot(bool on) {
    if(on) {
        state->viewerState->datasetMagLock = true;
    } else {
        state->viewerState->datasetMagLock = false;
    }
}

/**
  * @todo updateViewports
  */
void ZoomAndMultiresWidget::zoomDefaultsSlot() {

    state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = VPZOOMMIN;
    state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = VPZOOMMIN;
    state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = VPZOOMMIN;
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    //Viewer::refreshViewports();

}

void ZoomAndMultiresWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
