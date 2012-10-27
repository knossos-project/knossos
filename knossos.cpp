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
    QSplashScreen splashScreen(QPixmap("blue.jpeg")); // TODO
    splashScreen.setGeometry(QRect(100, 100, 500, 500)); // TODO Splash screen size
    splashScreen.setWindowFlags(Qt::WindowStaysOnTopHint);
    splashScreen.showMessage("Loading ..");
    splashScreen.show();

    MainWindow window;

    /* determining the display time for the splash */
    QTimer::singleShot(2000, &splashScreen, SLOT(close()));
    QTimer::singleShot(2000, &window, SLOT(show()));

    /* QTimer is async, so it has to be waiten manually*/
    QMutex mutex;
    mutex.lock();

    QWaitCondition waitCondition;
    waitCondition.wait(&mutex, 2000L);
    splashScreen.finish(&window);

    /* */




    return a.exec();
}

/* copied from http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer;  */
uint32_t log2uint32(register uint32_t x) {

    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);

    return(ones32(x >> 1));
}

/* http://aggregate.org/MAGIC/#Log2%20of%20an%20Integer */
uint32_t ones32(register uint32_t x) {
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
