#ifndef IOBSERVER_H
#define IOBSERVER_H

class IObserver
{
public:
    IObserver();
    virtual void update(void *data) = 0;
};

#endif // IOBSERVER_H
