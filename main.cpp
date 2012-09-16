#include <QtGui/QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "mainwindow.h"

//IMPORTANT. SDL redefines main
#ifdef main
#undef main
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /* displaying the splash */
    QSplashScreen splashScreen(QPixmap("blue.jpeg")); // TODO
    splashScreen.setGeometry(QRect(100, 100, 500, 500)); // TODO Splash screen size
    splashScreen.setWindowFlags(Qt::WindowStaysOnTopHint);
    splashScreen.showMessage("Loading ..");
    splashScreen.show();

    MainWindow w;

    /* determining the display time for the splash */
    QTimer::singleShot(2000, &splashScreen, SLOT(close()));
    QTimer::singleShot(2000, &w, SLOT(show()));

    /* QTimer is async, so it has to be waiten manually*/
    QMutex mutex;
    mutex.lock();

    QWaitCondition waitCondition;
    waitCondition.wait(&mutex, 2000L);
    splashScreen.finish(&w);

    return a.exec();
}
