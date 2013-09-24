#include "testskeletonviewport.h"
#include "widgets/viewport.h"

TestSkeletonViewport::TestSkeletonViewport(QObject *parent) :
    QObject(parent)
{
}

void TestSkeletonViewport::testTranslation() {
    Viewport *skeletonViewport = reference->vp4;
    int x = skeletonViewport->geometry().x() + 5;
    int y = skeletonViewport->geometry().y() + 5;

}
