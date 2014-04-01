#include "gui.h"
#include <QString>
#include <QStringList>
#include "knossos-global.h"

namespace gui {
    char settingsFile[2048];
    char titleString[2048];

// Current position of the user crosshair,
//starting at 1 instead 0. This is shown to the user,
//KNOSSOS works internally with 0 start indexing.
    Coordinate oneShiftedCurrPos;
    Coordinate activeNodeCoord;

    QString lockComment;
    char *commentBuffer = 0;
    char *commentSearchBuffer = 0;
    char *treeCommentBuffer = 0;

    int useLastActNodeRadiusAsDefault;
    float actNodeRadius;

// dataset navigation settings win buffer variables
    uint stepsPerSec;
    uint recenteringTime;
    uint dropFrames;

    char *comment1 = 0;
    char *comment2 = 0;
    char *comment3 = 0;
    char *comment4 = 0;
    char *comment5 = 0;

// substrings for comment node highlighting
    QStringList *commentSubstr = 0;
//char **commentSubstr;
// colors of color-dropdown in comment node highlighting
    char **commentColors = 0;

    void init() {

    }
}
