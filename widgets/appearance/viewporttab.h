#ifndef VIEWPORTTAB_H
#define VIEWPORTTAB_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

class ViewportTab : public QWidget
{
    friend class AppearanceWidget;
    friend class ViewportBase;
    friend class MainWindow;
    Q_OBJECT
    QGridLayout mainLayout;
    QVBoxLayout generalLayout;
    QFrame separator;
    QLabel generalHeader{"<strong>General</strong>"};
    QCheckBox showScalebarCheckBox{"Show scalebar"};
    QCheckBox showVPDecorationCheckBox{"Show viewport decorations"};
    QCheckBox drawIntersectionsCrossHairCheckBox{"Draw intersections crosshairs"};
    // 3D viewport
    QVBoxLayout viewport3DLayout;
    QLabel viewport3DHeader{"<strong>3D Viewport</strong>"};
    QCheckBox showXYPlaneCheckBox{"Show XY Plane"};
    QCheckBox showXZPlaneCheckBox{"Show XZ Plane"};
    QCheckBox showZYPlaneCheckBox{"Show ZY Plane"};
    QButtonGroup boundaryGroup;
    QRadioButton boundariesPixelRadioBtn{"Display dataset boundaries in pixels"};
    QRadioButton boundariesPhysicalRadioBtn{"Display dataset boundaries in µm"};
    QButtonGroup rotationCenterGroup;
    QRadioButton rotateAroundDatasetCenterRadioBtn{"Rotate around dataset center"};
    QRadioButton rotateAroundActiveNodeRadioBtn{"Rotate around active Node"};
    QRadioButton rotateAroundCurrentPositionRadioBtn{"Rotate around current position"};
    QPushButton resetVPsButton{"Reset viewport positions and sizes"};

    void saveSettings(QSettings & settings) const;
    void loadSettings(const QSettings & settings);
public:
    explicit ViewportTab(QWidget *parent = 0);

signals:
    void setViewportDecorations(const bool);
    void resetViewportPositions();

public slots:

};

#endif // VIEWPORTTAB_H
