#include "cheatsheet.h"

#include <QFile>

Cheatsheet::Cheatsheet(QWidget *parent) : QDockWidget("Cheatsheet", parent) {
    setWidget(&browser);
    setObjectName(windowTitle()); // unique name for QMainWindow::saveState()/restoreState()
    QObject::connect(&browser, &QTextBrowser::anchorClicked, this, &Cheatsheet::anchorClicked);
}

void Cheatsheet::load(const AnnotationMode mode) {
    QFile file(tr(":cheatsheet/%1.html").arg(
                   (mode == AnnotationMode::Mode_Merge) ? "mergemode" :
                   (mode == AnnotationMode::Mode_MergeTracing) ? "mergetracingmode" :
                   (mode == AnnotationMode::Mode_Paint) ? "paintmode" :
                   (mode == AnnotationMode::Mode_OverPaint) ? "overpaintmode" :
                   (mode == AnnotationMode::Mode_CellSegmentation) ? "segmentationcellpaintingmode" :
                   (mode == AnnotationMode::Mode_Tracing) ? "tracingmode" :
                   (mode == AnnotationMode::Mode_TracingAdvanced) ? "tracingadvancedmode" :
                   /*mode == AnnotationMode::Mode_Selection:*/ "reviewmode"));
    file.open(QFile::ReadOnly);
    QString page(file.readAll());
    browser.setHtml(page); // setSource() loses css styling. why? idk.
}
