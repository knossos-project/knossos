#ifndef GUI_H
#define GUI_H

#include <QObject>

class GUI : public QObject
{
    Q_OBJECT
public:
    explicit GUI(QObject *parent = 0);
    static void UI_saveSkeleton(int32_t increment);
    
signals:
    
public slots:
    
};

#endif // GUI_H
