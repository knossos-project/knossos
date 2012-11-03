#include "gui.h"

GUI::GUI(QObject *parent) :
    QObject(parent)
{
}

bool GUI::addRecentFile(char *path, uint32_t pos) { return true;}
void GUI::UI_saveSkeleton(int32_t increment) {}
bool GUI::cpBaseDirectory(char *target, char *path, size_t len) { return true;}
