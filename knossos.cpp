#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "mainwindow.h"
#include "knossos-global.h"

#include "knossos.h"
/*
#include "../treeLUT_fallback.h"
#include "../y.tab.h"
#include "../lex.yy.h"
*/

//static uint32_t isPathString(char *string);
//static uint32_t printUsage();
static int32_t initStates() { return 0;}
static int32_t printConfigValues() { return 0;}
static uint32_t cleanUpMain() { return 0;}
static int32_t tempConfigDefaults() { return 0;}
static struct stateInfo *emptyState() { return NULL;}
static int32_t readDataConfAndLocalConf() { return 0;}
static int32_t stripNewlines(char *string) { return 0;}
static int32_t configFromCli(int argCount, char *arguments[]) { return 0;}
static int32_t loadNeutralDatasetLUT(GLuint *lut) { return 0;}

int32_t readConfigFile(char *path) { return 0;}
static int32_t findAndRegisterAvailableDatasets() { return 0;}
#ifdef LINUX
static int32_t catchSegfault(int signum) { return 0;}
#endif

//IMPORTANT. SDL redefines main
#ifdef main
#undef main
#endif

struct stateInfo *tempConfig = NULL;
struct stateInfo *state = NULL;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /* displaying the splash */
    MainWindow window;
    window.showMaximized();

    QSplashScreen splashScreen(QPixmap("../splash"), Qt::WindowStaysOnTopHint);
    splashScreen.show();


    return a.exec();
}

/* http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer */
uint32_t Knossos::ones32(register uint32_t x) {
        /* 32-bit recursive reduction using SWAR...
       but first step is mapping 2-bit values
       into sum of 2 1-bit values in sneaky way
    */
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}


/* copied from http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer;  */
uint32_t Knossos::log2uint32(register uint32_t x) {

    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);

    return(ones32(x >> 1));
}

bool Knossos::lockSkeleton(int32_t targetRevision) { return true;}

bool Knossos::unlockSkeleton(int32_t increment) { return true;}

bool Knossos::sendClientSignal() { return true;}

bool Knossos::sendRemoteSignal() { return true;}

void Knossos::sendDatasetChangeSignal(uint32_t upOrDownFlag) {}

bool Knossos::sendLoadSignal(uint32_t x, uint32_t y, uint32_t z) { return true;}

bool Knossos::sendQuitSignal() { return true;}
