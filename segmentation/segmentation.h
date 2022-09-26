/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include "coordinate.h"
#include "hash_list.h"
#include "segmentationsplit.h"

#include <QColor>
#include <QDebug>
#include <QSet>
#include <QString>
#include <QObject>

#include <array>
#include <atomic>
#include <boost/optional.hpp>
#include <functional>
#include <limits>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>

enum SegmentationColor {
    Class,
    SubObject,
    Object
};

class Segmentation : public QObject {
Q_OBJECT
    friend void connectedComponent(const Coordinate & seed);
    friend void generateMeshesForSubobjectsOfSelectedObjects();
    friend void verticalSplittingPlane(const Coordinate & seed);
    friend auto & objectFromId(const quint64 objId);
    friend void selectMeshesForObjects();
    friend class SegmentationObjectModel;
    friend class TouchedObjectModel;
    friend class CategoryDelegate;
    friend class CategoryModel;
    friend class SegmentationView;
    friend class SegmentationProxy;

    class Object;
    class SubObject {
        friend void connectedComponent(const Coordinate & seed);
        friend void verticalSplittingPlane(const Coordinate & seed);
        friend class SegmentationObjectModel;
        friend class Segmentation;
        static inline uint64_t highestId{0};
        std::vector<uint64_t> objects;
        std::size_t selectedObjectsCount = 0;
    public:
        const auto & oidxs() const {
            return objects;
        }
        const uint64_t id;
        explicit SubObject(const uint64_t & id) : id(id) {
            highestId = std::max(id, highestId);
            objects.reserve(10);//improves merging performance by a factor of 3
        }
        SubObject(SubObject &&) = delete;
        SubObject(const SubObject &) = delete;
    };

    template<typename T, typename U>
    friend bool operator<(const std::reference_wrapper<T> & lhs, const std::reference_wrapper<U> & rhs) {
        return lhs.get().id < rhs.get().id;
    }
    template<typename T, typename U>
    friend bool operator==(const std::reference_wrapper<T> & lhs, const std::reference_wrapper<U> & rhs) {
        return lhs.get().id == rhs.get().id;
    }

    class Object {
        friend void connectedComponent(const Coordinate & seed);
        friend void verticalSplittingPlane(const Coordinate & seed);
        friend class SegmentationObjectModel;
        friend class TouchedObjectModel;
        friend class SegmentationView;
        friend class SegmentationProxy;
        friend class Segmentation;

        static inline std::uint64_t highestId{0};
        static inline std::uint64_t highestIndex{std::numeric_limits<std::uint64_t>::max()};
    public:
        //see http://coliru.stacked-crooked.com/a/aba85777991b4425
        std::vector<std::reference_wrapper<SubObject>> subobjects;
        uint64_t id;
        uint64_t index = ++highestIndex;
        bool todo;
        bool immutable;
        Coordinate location;
        QString category;
        boost::optional<std::tuple<uint8_t, uint8_t, uint8_t>> color;
        QString comment;
        bool selected = false;

        explicit Object(std::vector<std::reference_wrapper<SubObject>> initialVolumes, const Coordinate & location, const uint64_t id = ++highestId, const bool & todo = false, const bool & immutable = false);
        explicit Object(Object & first, Object & second);
        bool operator==(const Object & other) const;
        void addExistingSubObject(SubObject & sub);
        Object & merge(Object & other);
    };

    std::unordered_map<uint64_t, SubObject> subobjects;
    std::vector<Object> objects;
    std::unordered_map<uint64_t, uint64_t> objectIdToIndex;
    hash_list<uint64_t> selectedObjectIndices;
    const QSet<QString> prefixed_categories = {"", "cell", "cytoplasm", "ecs", "mito", "myelin", "neuron", "nucleus", "synapse"};
    QSet<QString> categories = prefixed_categories;
    uint64_t backgroundId = 0;
    uint64_t hovered_subobject_id = 0;
    // Selection via subobjects touches all objects containing the subobject.
    uint64_t touched_subobject_id = 0;
    // For segmentation job mode
    uint64_t lastTodoObject_id = 0;
    bool lockNewObjects{false};
    QString defaultMergeClass;
    // This array holds the table for overlay coloring.
    // The colors should be "maximally different".
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> overlayColorMap;

    Object & createObjectFromSubobjectId(const uint64_t initialSubobjectId, const Coordinate & location, uint64_t objectId = ++Object::highestId, const bool todo = false, const bool immutable = false);
    template<typename... Args>
    Object & createObject(Args && ... args);
    void removeObject(Object &);
    void changeCategory(Object & obj, const QString & category);
    void changeColor(Object & obj, const std::tuple<uint8_t, uint8_t, uint8_t> & color);
    void changeComment(Object & obj, const QString & comment);
    void newSubObject(Object & obj, uint64_t subObjID);

    void unmergeObject(Object & object, Object & other, const Coordinate & position);

    Object & objectFromSubobject(Segmentation::SubObject & subobject, const Coordinate & position);
public:
    class Job {
    public:
        bool active = false;
        int id;
        QString campaign;
        QString worker;
        QString path;
        QString submitPath;
    };
    Job job;
    void jobLoad(QIODevice & file);
    void jobSave(QIODevice & file) const;
    void startJobMode();
    void selectNextTodoObject();
    void selectPrevTodoObject();
    void markSelectedObjectForSplitting(const Coordinate & pos);

    bool renderOnlySelectedObjs{false};
    uint8_t alpha;
    brush_subject brush;
    bool createPaintObject{true};
    // for mode in which edges are online highlighted for objects when selected and being hovered over by mouse
    bool hoverVersion{false};
    uint64_t mouseFocusedObjectId{0};
    bool highlightBorder{true}; //highlight borders in segmentation viewport
    SegmentationColor segmentationColor{SegmentationColor::Class};

    bool enabled{false};
    std::size_t layerId;

    static Segmentation & singleton();

    Segmentation();
    //rendering
    void setRenderOnlySelectedObjs(const bool onlySelected);
    decltype(backgroundId) getBackgroundId() const;
    void setBackgroundId(decltype(backgroundId));
    decltype(lockNewObjects) getLockNewObjects() const;
    void setLockNewObjects(const decltype(lockNewObjects));
    using color_t = std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>;
    color_t subobjectColor(const uint64_t subObjectID) const;
    color_t colorObjectFromIndex(const uint64_t objectIndex) const;
    color_t colorOfSelectedObject() const;
    color_t colorOfSelectedObject(const SubObject & subobject) const;
    color_t colorObjectFromSubobjectId(const uint64_t subObjectID) const;
    //volume rendering
    bool volume_render_toggle = false;
    std::atomic_bool volume_update_required{false};
    uint volume_tex_id = 0;
    int volume_tex_len = 128;
    int volume_mouse_move_x = 0;
    int volume_mouse_move_y = 0;
    float volume_mouse_zoom = 1.0f;
    uint8_t volume_opacity = 255;
    QColor volume_background_color{Qt::darkGray};
    //data query
    bool hasObjects() const;
    bool hasSegData() const;
    bool subobjectExists(const uint64_t & subobjectId) const;
    uint64_t oid(const uint64_t oidx) const;
    const auto & highestSubobjectId() const {
        return SubObject::highestId;
    }
    //data access
    void createAndSelectObject(const Coordinate & position, const QString & category = "");
    SubObject & subobjectFromId(const uint64_t & subobjectId, const Coordinate & location);
    uint64_t subobjectIdOfFirstSelectedObject(const Coordinate & newLocation);
    bool objectOrder(const uint64_t &lhsIndex, const uint64_t &rhsIndex) const;
    uint64_t largestObjectContainingSubobjectId(const uint64_t subObjectId, const Coordinate & location);
    uint64_t largestObjectContainingSubobject(const SubObject & subobject) const;
    uint64_t tryLargestObjectContainingSubobject(const uint64_t subObjectId) const;
    uint64_t smallestImmutableObjectContainingSubobject(const SubObject & subobject) const;
    decltype(defaultMergeClass) getDefaultMergeClass() const;
    void setDefaultMergeClass(const QString & mergeClass);
    //selection query
    bool isSelected(const SubObject & rhs) const;
    bool isSelected(const uint64_t &objectIndex) const;
    bool isSubObjectIdSelected(const uint64_t & subobjectId) const;
    std::size_t selectedObjectsCount() const;
    //selection modification
    void selectObject(const uint64_t & objectIndex, const boost::optional<Coordinate> position = boost::none);
    void selectObject(Object & object);
    void selectObjectFromSubObject(SubObject &subobject, const Coordinate & position);
    void selectObjectFromSubObject(const uint64_t soid, const Coordinate & position);
    void unselectObject(const uint64_t & objectIndex);
    void unselectObject(Object & object);
    void clearObjectSelection();

    void setObjectLocation(const uint64_t, const Coordinate & position);

    void jumpToObject(const uint64_t & objectIndex);
    void jumpToObject(Object & object);
    std::vector<std::reference_wrapper<Segmentation::Object>> todolist();

    void hoverSubObject(const uint64_t subobject_id);
    void touchObjects(const uint64_t subobject_id);
    void untouchObjects();
    std::vector<std::reference_wrapper<Object>> touchedObjects();
    //custom modes
    void cell(bool nuc);
    void plusNuc();
    //files
    void mergelistClear();
    void mergelistSave(QIODevice & file) const;
    void mergelistSave(QTextStream & stream) const;
    void mergelistLoad(QIODevice & file);
    void mergelistLoad(QTextStream & stream);
    void loadOverlayLutFromFile(const QString & filename = ":/resources/color_palette/default.json");
    template<typename Func>
    void bulkOperation(Func func) {
        {
            QSignalBlocker b{this};
            func();
        }
        emit resetData();
    }
signals:
    void beforeAppendRow();
    void beforeRemoveRow();
    void appendedRow();
    void removedRow();
    void changedRow(int index);
    void changedRowSelection(int index);
    void merged(const quint64 index1, const quint64 index2); // used in plugin dfs_merge
    void unmerged(const quint64 index1, const quint64 index2); // used in plugin dfs_merge
    void selectionChanged();
    void resetData();
    void resetSelection();
    void resetTouchedObjects();
    void renderOnlySelectedObjsChanged(bool onlySelected);
    void backgroundIdChanged(uint64_t backgroundId);
    void lockNewObjectsChanged(const bool lockNewObjects);
    void categoriesChanged();
    void todosLeftChanged();
    void hoveredSubObjectChanged(const uint64_t subobject_id, const std::vector<uint64_t> & overlapObjects);
    void volumeRenderToggled(const bool render);
    void defaultMergeClassChanged(const QString & mergeClass);
public slots:
    void clear();
    void deleteSelectedObjects();
    void mergeSelectedObjects();
    void unmergeSelectedObjects(const Coordinate & clickPos);
    void jumpToSelectedObject();
    bool placeCommentForSelectedObjects(const QString & comment);
    void setSelectedObjectsMutability(bool immutable);
    void restoreDefaultColorForSelectedObjects();
    void generateColors();
    void toggleVolumeRender(const bool render);
};
