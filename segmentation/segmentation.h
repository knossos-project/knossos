#ifndef SEGMENTATION_H
#define SEGMENTATION_H

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
#include <functional>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>

class Segmentation : public QObject {
Q_OBJECT
    friend void connectedComponent(const Coordinate & seed);
    friend void verticalSplittingPlane(const Coordinate & seed);
    friend class SegmentationObjectModel;
    friend class TouchedObjectModel;
    friend class CategoryDelegate;
    friend class CategoryModel;
    friend class SegmentationTab;
    friend class SegmentationProxy;

    class Object;
    class SubObject {
        friend void connectedComponent(const Coordinate & seed);
        friend void verticalSplittingPlane(const Coordinate & seed);
        friend class SegmentationObjectModel;
        friend class Segmentation;
        static uint64_t highestId;
        std::vector<uint64_t> objects;
        std::size_t selectedObjectsCount = 0;
    public:
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
        friend class SegmentationTab;
        friend class Segmentation;

        static uint64_t highestId;
        static uint64_t highestIndex;

        //see http://coliru.stacked-crooked.com/a/aba85777991b4425
        std::vector<std::reference_wrapper<SubObject>> subobjects;
    public:
        uint64_t id;
        uint64_t index = ++highestIndex;
        bool todo;
        bool immutable;
        Coordinate location;
        QString category;
        QString comment;
        bool selected = false;

        explicit Object(SubObject & initialVolume);
        explicit Object(const uint64_t & id, const bool & todo, const bool & immutable, const Coordinate & location, SubObject & initialVolume);
        explicit Object(const bool & todo, const bool & immutable, const Coordinate & location, std::vector<std::reference_wrapper<SubObject>> initialVolumes);
        explicit Object(Object &first, Object &second);
        bool operator==(const Object & other) const;
        void addExistingSubObject(SubObject & sub);
        Object & merge(Object & other);
    };

    std::unordered_map<uint64_t, SubObject> subobjects;
    std::vector<Object> objects;
    hash_list<uint64_t> selectedObjectIndices;
    const QSet<QString> prefixed_categories = {"", "ecs", "mito", "myelin", "neuron", "synapse"};
    QSet<QString> categories = prefixed_categories;
    uint64_t hovered_subobject_id = 0;
    // Selection via subobjects touches all objects containing the subobject.
    uint64_t touched_subobject_id = 0;
    // For segmentation job mode
    uint64_t lastTodoObject_id = 0;

    // This array holds the table for overlay coloring.
    // The colors should be "maximally different".
    std::array<std::array<uint8_t, 256>, 3> overlayColorMap = [](){
        std::array<std::array<uint8_t, 256>, 3> lut;
        std::mt19937 rng;
        std::uniform_int_distribution<> dist(0, 255);
        //interleaved spectral colors
        for (std::size_t i = 0; i < 256 - 6; ++i) {//account for icrements inside the loop
            lut[0][i] = 255;
            lut[1][i] = 0;
            lut[2][i] = dist(rng);
            ++i;
            lut[0][i] = dist(rng);
            lut[1][i] = 255;
            lut[2][i] = 0;
            ++i;
            lut[0][i] = 0;
            lut[1][i] = dist(rng);
            lut[2][i] = 255;
            ++i;
            lut[0][i] = 255;
            lut[1][i] = dist(rng);
            lut[2][i] = 0;
            ++i;
            lut[0][i] = dist(rng);
            lut[1][i] = 0;
            lut[2][i] = 255;
            ++i;
            lut[0][i] = 0;
            lut[1][i] = 255;
            lut[2][i] = dist(rng);
        }
        return lut;
    }();

    Object & createObject(const uint64_t initialSubobjectId, const Coordinate & location);
    Object & createObject(const uint64_t initialSubobjectId, const Coordinate & location, const uint64_t & id, const bool & todo = false, const bool & immutable = false);
    void removeObject(Object &);
    void changeCategory(Object & obj, const QString & category);
    void changeComment(Object & obj, const QString & comment);
    void newSubObject(Object & obj, uint64_t subObjID);

    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> subobjectColor(const uint64_t subObjectID) const;

    Object & const_merge(Object & one, Object & other);
    void unmergeObject(Object & object, Object & other, const Coordinate & position);
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

    bool renderAllObjs; // show all segmentations as opposed to only a selected one
    uint8_t alpha;
    brush_t brush;
    // for mode in which edges are online highlighted for objects when selected and being hovered over by mouse
    bool hoverVersion;
    uint64_t mouseFocusedObjectId;

    static Segmentation & singleton();
    Segmentation();
    //rendering
    void setRenderAllObjs(bool);
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorOfSelectedObject(const SubObject & subobject) const;
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorObjectFromId(const uint64_t subObjectID) const;
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
    //data access
    void createAndSelectObject(const Coordinate & position);
    SubObject & subobjectFromId(const uint64_t & subobjectId, const Coordinate & location);
    uint64_t subobjectIdOfFirstSelectedObject();
    bool objectOrder(const uint64_t &lhsIndex, const uint64_t &rhsIndex) const;
    uint64_t largestObjectContainingSubobject(const SubObject & subobject) const;
    uint64_t tryLargestObjectContainingSubobject(const uint64_t subObjectId) const;
    uint64_t smallestImmutableObjectContainingSubobject(const SubObject & subobject) const;
    //selection query
    bool isSelected(const SubObject & rhs) const;
    bool isSelected(const uint64_t &objectIndex) const;
    bool isSubObjectIdSelected(const uint64_t & subobjectId) const;
    std::size_t selectedObjectsCount() const;
    //selection modification
    void selectObject(const uint64_t & objectIndex);
    void selectObject(Object & object);
    void selectObjectFromSubObject(SubObject &subobject, const Coordinate & position);
    void unselectObject(const uint64_t & objectIndex);
    void unselectObject(Object & object);
    void clearObjectSelection();

    void jumpToObject(const uint64_t & objectIndex);
    void jumpToObject(Object & object);
    std::vector<std::reference_wrapper<Segmentation::Object>> todolist();

    void updateLocationForFirstSelectedObject(const Coordinate & newLocation);

    void hoverSubObject(const uint64_t subobject_id);
    void touchObjects(const uint64_t subobject_id);
    void untouchObjects();
    std::vector<std::reference_wrapper<Object>> touchedObjects();
    //files
    void mergelistSave(QIODevice & file) const;
    void mergelistLoad(QIODevice & file);
    void loadOverlayLutFromFile(const std::string & filename = "stdOverlay.lut");
signals:
    void beforeAppendRow();
    void beforeRemoveRow();
    void appendedRow();
    void removedRow();
    void changedRow(int index);
    void resetData();
    void resetSelection();
    void resetTouchedObjects();
    void renderAllObjsChanged(bool all);
    void setRecenteringPositionSignal(float x, float y, float z);
    void categoriesChanged();
    void todosLeftChanged();
    void hoveredSubObjectChanged(const uint64_t subobject_id);
public slots:
    void clear();
    void deleteSelectedObjects();
    void mergeSelectedObjects();
    void unmergeSelectedObjects(const Coordinate & clickPos);
    void jumpToSelectedObject();
    void placeCommentForSelectedObject(const QString & comment);
};

#endif // SEGMENTATION_H
