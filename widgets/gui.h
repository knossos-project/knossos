#ifndef GUI_H
#define GUI_H
#include <QObject>

class Coordinate;
class QString;
class QStringList;

namespace gui {
    extern char settingsFile[2048];
    extern char titleString[2048];

    // Current position of the user crosshair,
    //starting at 1 instead 0. This is shown to the user,
    //KNOSSOS works internally with 0 start indexing.
    extern Coordinate oneShiftedCurrPos;
    extern Coordinate activeNodeCoord;

    extern QString lockComment;
    extern char *commentBuffer;
    extern char *commentSearchBuffer;
    extern char *treeCommentBuffer;

    extern int useLastActNodeRadiusAsDefault;
    extern float actNodeRadius;

    // dataset navigation settings win buffer variables
    extern uint stepsPerSec;
    extern uint recenteringTime;
    extern uint dropFrames;

    extern char *comment1;
    extern char *comment2;
    extern char *comment3;
    extern char *comment4;
    extern char *comment5;

    // substrings for comment node highlighting
    extern QStringList *commentSubstr;
    //char **commentSubstr;
    // colors of color-dropdown in comment node highlighting
    extern char **commentColors;
    extern void init();
}

#endif // GUI_H
