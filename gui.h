#ifndef GUI_H
#define GUI_H

#include <QObject>

class GUI : public QObject
{
    Q_OBJECT
public:
    explicit GUI(QObject *parent = 0);

    static bool addRecentFile(char *path, uint32_t pos);
    static void UI_saveSkeleton(int32_t increment);
    static bool cpBaseDirectory(char *target, char *path, size_t len);

    //TODO AGAR
    static void yesNoPrompt(void *par, char *promptString, void (*yesCb)(), void (*noCb)());
    
signals:
    
public slots:
    
};

#endif // GUI_H
