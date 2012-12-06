#ifndef SLEEPER_H
#define SLEEPER_H

#include <QThread>

class Sleeper : public QThread {
public:
    explicit Sleeper(QThread *parent = 0);
    static void msleep(int ms);
};

#endif // SLEEPER_H
