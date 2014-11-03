#include <QtWidgets>

#include "datasetloadtabwidget.h"
#include "datasetload/datasetlocalwidget.h"
#include "datasetload/datasetremotewidget.h"

#include "widgets/GuiConstants.h"
#include "knossos-global.h"

DatasetLoadTabWidget::DatasetLoadTabWidget(QWidget *parent)
    : QDialog(parent)
{
    datasetLocalWidget = new DatasetLocalWidget;
    datasetRemoteWidget = new DatasetRemoteWidget;

    tabWidget = new QTabWidget;

    tabWidget->addTab(datasetLocalWidget, tr("Local"));
    tabWidget->addTab(datasetRemoteWidget, tr("Remote"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);

    setWindowTitle(tr("Load Dataset"));

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetLoadTabWidget::saveSettings() {

    QSettings settings;\
    //Dataset LOCAL
    settings.beginGroup(DATASET_WIDGET);
    settings.setValue(DATASET_MRU, datasetLocalWidget->getRecentPathItems());

    //Dataset REMOTE

    std::string url{std::string(state->ftpHostName) + std::string(state->ftpBasePath)};

    if(url.empty()) {
        settings.endGroup();
        return;
    }

    if(url.find("/n") != std::string::npos) {
        url.erase(url.find("/n"), 2);
    }

    settings.setValue(DATASET_URL, url.c_str());
    settings.setValue(DATASET_USER, state->ftpUsername);
    settings.setValue(DATASET_PWD, state->ftpPassword);

    settings.setValue(DATASET_M, state->M);
    settings.setValue(DATASET_OVERLAY, state->overlay);
    settings.endGroup();

}


void DatasetLoadTabWidget::loadSettings() {
    QSettings settings;
    //dataset LOCAL
    settings.beginGroup(DATASET_WIDGET);

    datasetLocalWidget->pathDropdown->clear();
    datasetLocalWidget->pathDropdown->insertItems(0, settings.value(DATASET_MRU).toStringList());

    if (QApplication::arguments().filter("supercube-edge").empty()) {//if not provided by cmdline
        state->M = settings.value(DATASET_M, 3).toInt();
    }
    if (QApplication::arguments().filter("overlay").empty()) {//if not provided by cmdline
        state->overlay = settings.value(DATASET_OVERLAY, true).toBool();
    }


    datasetLocalWidget->supercubeEdgeSpin->setValue(state->M);
    datasetLocalWidget->segmentationOverlayCheckbox.setCheckState(state->overlay ? Qt::Checked : Qt::Unchecked);
    datasetLocalWidget->adaptMemoryConsumption();


    //dataset REMOTE

    datasetRemoteWidget->urlField->setText(settings.value(DATASET_URL).toString());
    datasetRemoteWidget->usernameField->setText(settings.value(DATASET_USER).toString());
    datasetRemoteWidget->passwordField->setText(settings.value(DATASET_PWD).toString());

    settings.endGroup();


    //settings depending on M
    state->cubeSetElements = state->M * state->M * state->M;
    state->cubeSetBytes = state->cubeSetElements * state->cubeBytes;

    for(uint i = 0; i < state->viewerState->numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= static_cast<float>(state->magnification);
        state->viewerState->vpConfigs[i].texture.usedTexLengthDc = state->M;
    }

    if(state->M * state->cubeEdgeLength >= TEXTURE_EDGE_LEN) {
        qDebug() << "Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!";
        throw std::runtime_error("Please choose smaller values for M or N. Your choice exceeds the KNOSSOS texture size!");
    }

    // We're not doing stuff in parallel, yet. So we skip the locking
    // part.
    // This *10 thing is completely arbitrary. The larger the table size,
    // the lower the chance of getting collisions and the better the loading
    // order will be respected. *10 doesn't seem to have much of an effect
    // on performance but we should try to find the optimal value some day.
    // Btw: A more clever implementation would be to use an array exactly the
    // size of the supercube and index using the modulo operator.
    // sadly, that realization came rather late. ;)

    // creating the hashtables is cheap, keeping the datacubes is
    // memory expensive..
    for(int i = 0; i <= NUM_MAG_DATASETS; i = i * 2) {
        state->Dc2Pointer[int_log(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
        state->Oc2Pointer[int_log(i)] = Hashtable::ht_new(state->cubeSetElements * 10);
        if(i == 0) i = 1;
    }
}


