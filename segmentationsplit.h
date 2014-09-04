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
signals:
    void modeChanged(const mode_t);
    void radiusChanged(const int);
    void toolChanged(const tool_t);
private:
    mode_t mode = mode_t::two_dim;
    int radius = 10;
    tool_t tool = tool_t::merge;
};

void verticalSplittingPlane(const Coordinate & seed);

#endif//SEGMENTATION_SPLIT_H
