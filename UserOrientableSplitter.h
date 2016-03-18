#ifndef USER_ORIENTABLE_SPLITTER_H
#define USER_ORIENTABLE_SPLITTER_H

#include <QSplitter>

class UserOrientableSplitter : public QSplitter {
    virtual QSplitterHandle * createHandle() override;
};

#endif//USER_ORIENTABLE_SPLITTER_H
