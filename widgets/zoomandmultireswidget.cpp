#include "zoomandmultireswidget.h"

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
    this->orthogonalDataViewportSpinBox->setMaximum(1.00);
    this->orthogonalDataViewportSpinBox->setMinimum(0.02);
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
    this->currentActiveMagDatasetLabel = new QLabel("Currently active mag dataset:");
    this->highestActiveMagDatasetLabel = new QLabel("Highest active mag dataset:");
    this->lowestActiveMagDatasetLabel = new QLabel("Lowest active mag dataset:");

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

    }
}

void ZoomAndMultiresWidget::zoomDefaultsSlot() {

}

void ZoomAndMultiresWidget::closeEvent(QCloseEvent *event) {
    this->hide();
}
