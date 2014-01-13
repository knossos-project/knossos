#ifndef SKELETONIZERDECORATOR_H
#define SKELETONIZERDECORATOR_H

#include <QObject>
#include "knossos-global.h"
#include "skeletonizer.h"

class SkeletonizerDecorator : public QObject
{
    Q_OBJECT
public:
    explicit SkeletonizerDecorator(QObject *parent = 0);
    Skeletonizer *reference;
signals:
    void userMoveSignal(int x, int y, int z);
public slots:
    void aSlot();

};

#endif // SKELETONIZERDECORATOR_H
