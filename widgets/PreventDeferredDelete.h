#ifndef PREVENT_DEFERRED_DELETE_H
#define PREVENT_DEFERRED_DELETE_H

#include <QEvent>

template<typename T>
class PreventDeferredDelete : public T {
    virtual bool event(QEvent * event) override final {
        //disable implicit deletion
        if (event->type() == QEvent::DeferredDelete) {
            this->setParent(nullptr);
            event->accept();
            return true;
        } else {
            return T::event(event);
        }
    }
};

#endif//PREVENT_DEFERRED_DELETE_H
