#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include "coordinate.h"
#include "hash_list.h"

#include <QDebug>
#include <QString>
#include <QObject>

#include <array>
#include <functional>
#include <random>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

class Segmentation : public QObject {
Q_OBJECT
    friend class SegmentationObjectModel;
    friend class TouchedObjectModel;
    friend class CategoryModel;
    friend class SegmentationTab;

    class Object;
    class SubObject {
        friend class SegmentationObjectModel;
        friend class Segmentation;
        std::vector<uint64_t> objects;
        std::size_t selectedObjectsCount = 0;
    public:
        const uint64_t id;
        explicit SubObject(const uint64_t & id) : id(id) {
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
        friend class SegmentationObjectModel;
        friend class TouchedObjectModel;
        friend class SegmentationTab;
        friend class Segmentation;

        static uint64_t highestId;

        //see http://coliru.stacked-crooked.com/a/aba85777991b4425
        std::vector<std::reference_wrapper<SubObject>> subobjects;
    public:
        uint64_t id = ++highestId;
        bool immutable;
        QString category;
        QString comment;
        bool selected = false;

        explicit Object(SubObject & initialVolume);
        explicit Object(const bool & immutable, SubObject & initialVolume);
        explicit Object(const bool & immutable, std::vector<std::reference_wrapper<SubObject>> initialVolumes);
        explicit Object(Object &first, Object &second);
        bool operator==(const Object & other) const;
        void addExistingSubObject(SubObject & sub);
        Object & merge(Object & other);
    };

    std::unordered_map<uint64_t, SubObject> subobjects;
    std::vector<Object> objects;
    hash_list<uint64_t> selectedObjectIds;
    std::set<QString> categories = {"mito", "myelin", "neuron", "synapse"};
    // Selection via subobjects touches all objects containing the subobject.
    uint64_t touched_subobject_id = 0;
    bool renderAllObjs; // show all segmentations as opposed to only a selected one
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

    Object & createObject(const uint64_t initialSubobjectId, const bool & immutable = false);
    void removeObject(Object &);
    void newSubObject(Object & obj, uint64_t subObjID);

    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> subobjectColor(const uint64_t subObjectID) const;

    Object & const_merge(Object & one, Object & other);
    void unmergeObject(Object & object, Object & other);
public:
    uint8_t alpha;
    bool segmentationMode;
    // line merging: the drawn line merges all visited objects
    std::vector<Coordinate> mergeLine;

    static Segmentation & singleton();
    Segmentation();
    //color picking
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorUniqueFromId(const uint64_t subObjectID) const;
    uint64_t subobjectIdFromUniqueColor(std::tuple<uint8_t, uint8_t, uint8_t> color) const;
    //rendering
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorOfSelectedObject(const SubObject & subobject) const;
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> colorObjectFromId(const uint64_t subObjectID) const;
    //data query
    bool hasObjects() const;
    bool subobjectExists(const uint64_t & subobjectId) const;
    //data access
    SubObject & subobjectFromId(const uint64_t & subobjectId);
    bool objectOrder(const uint64_t &lhsId, const uint64_t &rhsId) const;
    Object &largestObjectContainingSubobject(const SubObject & subobject);
    const Object &largestObjectContainingSubobject(const SubObject & subobject) const;
    Object &smallestImmutableObjectContainingSubobject(const SubObject & subobject);
    const Object &smallestImmutableObjectContainingSubobject(const SubObject & subobject) const;
    //selection query
    bool isSelected(const SubObject & rhs) const;
    bool isSelected(const Object & rhs) const;
    std::size_t selectedObjectsCount() const;
    //selection modification
    void selectObject(const uint64_t & objectId);
    void selectObject(Object & object);
    void selectObjectFromSubObject(SubObject &subobject);
    void unselectObject(const uint64_t & objectId);
    void unselectObject(Object & object);
    void clearObjectSelection();

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
    void changedRow(int id);
    void resetData();
    void resetSelection();
    void resetTouchedObjects();
public slots:
    void clear();
    void deleteSelectedObjects();
    void mergeSelectedObjects();
    void unmergeSelectedObjects(SubObject & subobjectToUnmerge);
    void unmergeSelectedObjects(Object & objectToUnmerge);
};

#endif // SEGMENTATION_H
