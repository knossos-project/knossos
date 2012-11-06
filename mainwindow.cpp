
#include "mainwindow.h"
#include "ui_mainwindow.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("KnossosQT");

    createActions();
    createMenus();

    mainWidget = new QWidget(this);
    gridLayout = new QGridLayout();
    mainWidget->setLayout(gridLayout);
    setCentralWidget(mainWidget);

    glWidgets = new OpenGLWidget*[4];
    for(int i = 0; i < 4; i++) {
        glWidgets[i] = new OpenGLWidget(this, i);
        glWidgets[i]->setStyleSheet("background:black");
    }

    gridLayout->addWidget(glWidgets[0], 0, 1);
    gridLayout->addWidget(glWidgets[1], 0, 2);
    gridLayout->addWidget(glWidgets[2], 1, 1);
    gridLayout->addWidget(glWidgets[3], 1, 2);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions()
{
    /* file actions */
    openAction = new QAction(tr("&Open"), this);
    recentFileAction = new QAction(tr("&Recent Files"), this);
    saveAction = new QAction(tr("&Save (CTRL+s)"), this);
    saveAsAction = new QAction(tr("&Save as"), this);
    quitAction = new QAction(tr("&Quit"), this);


    connect(openAction, SIGNAL(triggered()), this, SLOT(openSlot()));
    connect(recentFileAction, SIGNAL(triggered()), this, SLOT(recentFilesSlot()));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveSlot()));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAsSlot()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quitSlot()));

    /* edit skeleton actions */
    addNodeAction = new QAction(tr("&Add Node"), this);
    linkWithActiveNodeAction = new QAction(tr("&Link with Active Node"), this);
    dropNodesAction = new QAction(tr("&Drop Nodes"), this);
    skeletonStatisticsAction = new QAction(tr("&Skeleton Statistics"), this);
    clearSkeletonAction =  new QAction(tr("&Clear Skeleton"), this);

    connect(addNodeAction, SIGNAL(triggered()), this, SLOT(addNodeSlot()));
    connect(linkWithActiveNodeAction, SIGNAL(triggered()), this, SLOT(linkWithActiveNodeSlot()));
    connect(dropNodesAction, SIGNAL(triggered()), this, SLOT(dropNodesSlot()));
    connect(skeletonStatisticsAction, SIGNAL(triggered()), this, SLOT(skeletonStatisticsSlot()));
    connect(clearSkeletonAction, SIGNAL(triggered()), this, SLOT(clearSkeletonSlot()));

    /* view actions */
    workModeViewAction = new QAction(tr("&Work Mode"), this);
    dragDatasetAction = new QAction(tr("&Drag Dataset"), this);
    recenterOnClickAction = new QAction(tr("&Recenter on Click"), this);
    zoomAndMultiresAction = new QAction(tr("Zoom and Multires.."), this);
    tracingTimeAction = new QAction(tr("&Tracing Time"), this);

    connect(workModeViewAction, SIGNAL(triggered()), this, SLOT(workModeViewSlot()));
    connect(dragDatasetAction, SIGNAL(triggered()), this, SLOT(dragDatasetSlot()));
    connect(recenterOnClickAction, SIGNAL(triggered()), this, SLOT(recenterOnClickSlot()));
    connect(zoomAndMultiresAction, SIGNAL(triggered()), this, SLOT(zoomAndMultiresSlot()));
    connect(tracingTimeAction, SIGNAL(triggered()), this, SLOT(tracingTimeSlot()));

    /* preferences actions */
    loadCustomPreferencesAction = new QAction(tr("&Load Custom Preferences"), this);
    saveCustomPreferencesAction = new QAction(tr("&Save Custom Preferences"), this);
    defaultPreferencesAction = new QAction(tr("&Default Preferences"), this);
    datasetNavigationAction = new QAction(tr("&Dataset Navigation"), this);
    synchronizationAction = new QAction(tr("&Synchronization"), this);
    dataSavingOptionsAction = new QAction(tr("&Data Saving Options"), this);
    viewPortSettingsAction = new QAction(tr("&Viewport Settings"), this);

    connect(loadCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(loadCustomPreferencesSlot()));
    connect(saveCustomPreferencesAction, SIGNAL(triggered()), this, SLOT(saveCustomPreferencesSlot()));
    connect(defaultPreferencesAction, SIGNAL(triggered()), this, SLOT(defaultPreferencesSlot()));
    connect(datasetNavigationAction, SIGNAL(triggered()), this, SLOT(datatasetNavigationSlot()));
    connect(dataSavingOptionsAction, SIGNAL(triggered()), this, SLOT(dataSavingOptionsSlot()));
    connect(viewPortSettingsAction, SIGNAL(triggered()), this, SLOT(viewPortSettingsSlot()));

    /* window actions */
    toolsAction = new QAction(tr("&Tools"), this);
    logAction = new QAction(tr("&Log"), this);
    commentShortcutsAction = new QAction(tr("&Comment Shortcuts"), this);

    connect(toolsAction, SIGNAL(triggered()), this, SLOT(toolsSlot()));
    connect(logAction, SIGNAL(triggered()), this, SLOT(logSlot()));
    connect(commentShortcutsAction, SIGNAL(triggered()), this, SLOT(commentShortcutsSlots()));

    /* Help actions */
    aboutAction = new QAction(tr("&About"), this);

    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutSlot()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(openAction);
    fileMenu->addAction(recentFileAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    editMenu = menuBar()->addMenu("&Edit Skeleton");
    workModeMenu = editMenu->addMenu("&Work Mode");
        workModeMenu->addAction(addNodeAction);
        workModeMenu->addAction(linkWithActiveNodeAction);
        workModeMenu->addAction(dropNodesAction);
    editMenu->addAction(skeletonStatisticsAction);
    editMenu->addAction(clearSkeletonAction);

    viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(workModeViewAction);
    viewMenu->addAction(dragDatasetAction);
    viewMenu->addAction(zoomAndMultiresAction);
    viewMenu->addAction(tracingTimeAction);

    preferenceMenu = menuBar()->addMenu("&Preferences");
    preferenceMenu->addAction(loadCustomPreferencesAction);
    preferenceMenu->addAction(saveCustomPreferencesAction);
    preferenceMenu->addAction(defaultPreferencesAction);
    preferenceMenu->addAction(datasetNavigationAction);
    preferenceMenu->addAction(synchronizationAction);
    preferenceMenu->addAction(dataSavingOptionsAction);
    preferenceMenu->addAction(viewPortSettingsAction);

    windowMenu = menuBar()->addMenu("&Windows");
    windowMenu->addAction(toolsAction);
    windowMenu->addAction(logAction);
    windowMenu->addAction(commentShortcutsAction);

    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    qApp->quit();
    /*if(state->skeletonState->unsavedChanges) {
        event->ignore();
        prompt = new QMessageBox(this);
        prompt->setWindowTitle("Confirmation");
        prompt->setText("There are unsaved changes. Really Quit?");

        QPushButton *yesButton = prompt->addButton(tr("Yes"), QMessageBox::ActionRole);
        QPushButton *noButton = prompt->addButton(tr("No"), QMessageBox::ActionRole);
        prompt->exec();

        if((QPushButton *) prompt->clickedButton() == yesButton) {
            event->accept();
        }

        if((QPushButton *) prompt->clickedButton() == noButton) {
            prompt->close();
        }
    }*/

}

/* file menu functionality */

void MainWindow::openSlot()
{

}

void MainWindow::recentFilesSlot()
{

}

void MainWindow::saveSlot()
{

}

void MainWindow::saveAsSlot()
{

}

void MainWindow::quitSlot()
{

}

/* edit skeleton functionality */

void MainWindow::workModeEditSlot()
{

}

void MainWindow::addNodeSlot()
{

}

void MainWindow::linkWithActiveNodeSlot()
{

}

void MainWindow::dropNodesSlot()
{

}


void MainWindow::skeletonStatisticsSlot()
{

}

void MainWindow::clearSkeletonSlot()
{

}

/* view menu functionality */

void MainWindow::workModeViewSlot()
{

}

void MainWindow::dragDatasetSlot()
{

}

void MainWindow::recenterOnClickSlot()
{

}

void MainWindow::zoomAndMultiresSlot()
{

}

void MainWindow::tracingTimeSlot()
{

}

/* preference menu functionality */

void MainWindow::loadCustomPreferencesSlot()
{

}

void MainWindow::saveCustomPreferencesSlot()
{

}

void MainWindow::defaultPreferencesSlot()
{

}

void MainWindow::datatasetNavigationSlot()
{

}

void MainWindow::synchronizationSlot()
{

}

void MainWindow::dataSavingOptionsSlot()
{

}

void MainWindow::viewPortSettingsSlot()
{

}

/* window menu functionality */

void MainWindow::toolsSlot()
{

}

void MainWindow::logSlot()
{

}

void MainWindow::commentShortcutsSlots()
{

}

/* help menu functionality */

void MainWindow::aboutSlot()
{

}

