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
        merge, add
    };
    enum class view_t {
        xy, xz, yz
    };
    enum class shape_t {
        square, circle
    };

    void setInverse(const bool newInverse) {
        inverse = newInverse;
        emit inverseChanged(inverse);
    }
    bool isInverse() const {
        return inverse;
    }
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
    void setShape(const shape_t newShape) {
        shape = newShape;
        emit shapeChanged(shape);
    }
    shape_t getShape() const {
        return shape;
    }
signals:
    void inverseChanged(const bool);
    void modeChanged(const mode_t);
    void radiusChanged(const int);
    void toolChanged(const tool_t);
    void shapeChanged(const shape_t);
private:
    int radius = 10;
    bool inverse = false;
    mode_t mode = mode_t::two_dim;
    tool_t tool = tool_t::merge;
    view_t view = view_t::xy;
    shape_t shape = shape_t::circle;
};

void connectedComponent(const Coordinate & seed);
void verticalSplittingPlane(const Coordinate & seed);

#endif//SEGMENTATION_SPLIT_H
