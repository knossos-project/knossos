#ifndef VIEWPORTTAB_H
#define VIEWPORTTAB_H

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class ViewportTab : public QWidget
{
    friend class AppearanceWidget;
    friend class EventModel;
    friend class MainWindow;
    Q_OBJECT
    QGridLayout mainLayout;
    QVBoxLayout generalLayout;
    QFrame separator;
    QLabel generalHeader{"<strong>General</strong>"};
    QCheckBox showScalebarCheckBox{"Show scalebar"};
    QCheckBox showVPDecorationCheckBox{"Show viewport decorations"};
    QCheckBox drawIntersectionsCrossHairCheckBox{"Draw intersections crosshairs"};
    QCheckBox arbitraryModeCheckBox{"Arbitrary viewport orientation"};
    // 3D viewport
    QVBoxLayout viewport3DLayout;
    QLabel viewport3DHeader{"<strong>3D Viewport</strong>"};
    QCheckBox showXYPlaneCheckBox{"Show XY Plane"};
    QCheckBox showXZPlaneCheckBox{"Show XZ Plane"};
    QCheckBox showYZPlaneCheckBox{"Show YZ Plane"};
    QRadioButton boundariesPixelRadioBtn{"Display dataset boundaries in pixels"};
    QRadioButton boundariesPhysicalRadioBtn{"Display dataset boundaries in µm"};
    QCheckBox rotateAroundActiveNodeCheckBox{"Rotate Around Active Node"};
    QPushButton resetVPsButton{"Reset viewport positions and sizes"};
public:
    explicit ViewportTab(QWidget *parent = 0);

signals:
    void showIntersectionsSignal(const bool value);
    void setVPOrientationSignal(const bool arbitrary);
    void setViewportDecorations(const bool);
    void resetViewportPositions();

public slots:

};

#endif // VIEWPORTTAB_H
