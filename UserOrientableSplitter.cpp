#include "UserOrientableSplitter.h"

class SplitterHandle : public QSplitterHandle {
    virtual void mouseDoubleClickEvent(QMouseEvent * event) override {
        QSplitterHandle::mouseDoubleClickEvent(event);
        splitter()->setOrientation(orientation() == Qt::Orientation::Horizontal ? Qt::Orientation::Vertical : Qt::Orientation::Horizontal);
    }
public:
    using QSplitterHandle::QSplitterHandle;
};

 QSplitterHandle * UserOrientableSplitter::createHandle() {
    return new SplitterHandle(orientation(), this);
}
