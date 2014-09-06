#ifndef SEGMENTATION_SPLIT_H
#define SEGMENTATION_SPLIT_H

#include "coordinate.h"

#include <QObject>

#include <set>

class brush_t : public QObject {
    Q_OBJECT
public:
    enum class mode_t {
        two_dim, three_dim
    };
    enum class tool_t {
        merge, add, erase
    };
    enum class view_t {
        xy, xz, yz
    };

    void setMode(const mode_t newMode) {
        mode = newMode;
        emit modeChanged(mode);
    }
    mode_t getMode() const {
        return mode;
    }
    void setRadius(const int newRadius) {
        radius = newRadius;
        emit radiusChanged(radius);
    }
    int getRadius() const {
        return radius;
    }
    void setTool(const tool_t newTool) {
        tool = newTool;
        emit toolChanged(tool);
    }
    tool_t getTool() const {
        return tool;
    }
    void setView(const view_t newView) {
        view = newView;
    }
    view_t getView() const {
        return view;
    }
signals:
    void modeChanged(const mode_t);
    void radiusChanged(const int);
    void toolChanged(const tool_t);
private:
    int radius = 10;
    mode_t mode = mode_t::two_dim;
    tool_t tool = tool_t::merge;
    view_t view = view_t::xy;
};

void connectedComponent(const Coordinate & seed);
void verticalSplittingPlane(const Coordinate & seed);

#endif//SEGMENTATION_SPLIT_H
