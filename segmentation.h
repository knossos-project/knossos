#ifndef SEGEMENTATION_H
#define SEGEMENTATION_H

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <QDebug>
#include <QString>
#include <QObject>

class Segmentation : public QObject {
Q_OBJECT
    friend class SegmentationObjectModel;
    friend class SegmentationTab;
    friend class Viewer;

    class Object;
    class SubObject {
        friend class Object;
        friend class Segmentation;
        friend class Viewer;
        std::vector<std::reference_wrapper<Object>> objects;
    public:
        uint64_t id;
        SubObject(const uint64_t id = 0) : id(id) {}
        SubObject(SubObject &&) = default;
        SubObject(const SubObject &) = delete;
    };

    friend bool operator<(const std::reference_wrapper<SubObject> & lhs, const std::reference_wrapper<SubObject> & rhs) {
        return &lhs.get() < &rhs.get();
    }
    
    class Object {
        friend class SegmentationObjectModel;
        friend class Segmentation;

        QString category;

        //see http://coliru.stacked-crooked.com/a/aba85777991b4425
        std::vector<std::reference_wrapper<SubObject>> subobjects;

        Object & merge(Object && other) {
            //replace reference
            for (auto & elem : other.subobjects) {
                std::replace_if(std::begin(elem.get().objects), std::end(elem.get().objects), [&](const std::reference_wrapper<Object> obj){
                    return obj.get().id == other.id;
                } , std::ref(*this));
            }
            //move subobjects
            decltype(subobjects) tmp;
            tmp.reserve(subobjects.size() + other.subobjects.size());
            std::sort(std::begin(subobjects), std::end(subobjects));
            std::sort(std::begin(other.subobjects), std::end(other.subobjects));
            std::merge(std::begin(subobjects), std::end(subobjects), std::begin(other.subobjects), std::end(other.subobjects), std::back_inserter(tmp));
            std::swap(subobjects, tmp);
            return *this;
        }
    public:
        uint64_t id;
        bool selected;
        Object() : id(0), selected(false) {}
        Object(Object &&) = delete;
        Object(const Object &) = delete;
        Object(SubObject & initialVolume) : id(initialVolume.id), selected(false) {
            initialVolume.objects.emplace_back(*this);//register parent
            subobjects.emplace_back(initialVolume);//add child
        }
        bool operator==(const Object & other) {
            return id == other.id;
        }
    };

    std::unordered_map<uint64_t, SubObject> subobjects;
    std::unordered_map<uint64_t, Object> objects;
    std::vector<std::reference_wrapper<Object>> selectedObjects;
    bool allObjs; // show all segmentations as opposed to only a selected one

    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> subobjectColor(const uint64_t subObjectID) {
        const uint8_t red   = overlayColorMap[0][subObjectID % 256];
        const uint8_t green = overlayColorMap[1][subObjectID % 256];
        const uint8_t blue  = overlayColorMap[2][subObjectID % 256];
        return std::make_tuple(red, green, blue, alpha);
    }
public:
    static Segmentation & singleton() {
        static Segmentation segmentation;
        return segmentation;
    }
    // This array holds the table for overlay coloring.
    // The colors should be "maximally different".
    std::array<std::array<uint8_t, 256>, 3> overlayColorMap = [](){
        std::array<std::array<uint8_t, 256>, 3> lut;
        //interleaved spectral colors
        for (std::size_t i = 0; i < 256; ++i) {
            lut[0][i] = i;
            lut[1][i] = 255;
            lut[2][i] = 0;
            ++i;
            lut[0][i] = i;
            lut[1][i] = 0;
            lut[2][i] = 255;
            ++i;
            lut[0][i] = 255;
            lut[1][i] = i;
            lut[2][i] = 0;
            ++i;
            lut[0][i] = 0;
            lut[1][i] = i;
            lut[2][i] = 255;
            ++i;
            lut[0][i] = 255;
            lut[1][i] = 0;
            lut[2][i] = i;
            ++i;
            lut[0][i] = 0;
            lut[1][i] = 255;
            lut[2][i] = i;
        }
        return lut;
    }();
    uint8_t alpha;
    Segmentation() : allObjs(false) {
        //generate meta data from raw segmentation
        parseCube("E:/segmentation test dataset/x0000/y0000/z0000/segmentation_x0000_y0000_z0000.raw.segmentation");
        parseCube("E:/segmentation test dataset/x0001/y0000/z0000/segmentation_x0001_y0000_z0000.raw.segmentation");
        parseCube("E:/segmentation test dataset/x0000/y0000/z0000/segmentation_x0000_y0000_z0000.raw.segmentation.raw");
        parseCube("E:/segmentation test dataset/x0001/y0000/z0000/segmentation_x0001_y0000_z0000.raw.segmentation.raw");
    }
    void parseCube(std::string path) {
        std::ifstream cubeFile(path, std::ios_base::binary);
        std::vector<uint64_t> cube(128 * 128 * 128);
        cubeFile.read(reinterpret_cast<char * const>(cube.data()), cube.size() * sizeof(cube[0]));
        cube = decltype(cube)(std::begin(cube), std::unique(std::begin(cube), std::end(cube)));
        for (const auto & id : cube) {
            if (subobjects.find(id) == std::end(subobjects)) {//insert only if not already present
                auto & newKeyValuePair = *subobjects.emplace(id, id).first;//first is iterator to the newly inserted key-value pair
                objects.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(newKeyValuePair.second));//create object from supervoxel
            }
        }
        emit dataChanged();
    }

    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> objectColorFromSubobject(const uint64_t subObjectID) {
        if (subobjects.empty()) {
            return subobjectColor(subObjectID);
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        uint8_t currAlpha = alpha;
        bool selected = false;
        const auto objectCount = subobjects[subObjectID].objects.size();
        for (const auto & object : subobjects[subObjectID].objects) {
            if(object.get().selected) {
                selected = true;
                if(allObjs) {
                    red = 255;
                    green = 0;
                    blue = 0;
                    currAlpha = 255;
                    break;
                }
            }
            const auto objectID = object.get().id;
            red += overlayColorMap[0][objectID % 256] / objectCount;
            green += overlayColorMap[1][objectID % 256] / objectCount;
            blue += overlayColorMap[2][objectID % 256] / objectCount;
        }
        if(allObjs == false && selected == false) {
            currAlpha = 0;
        }
        return std::make_tuple(red, green, blue, currAlpha);
    }

    Object & merge(Object & one, Object && other) {
        one.merge(std::move(other));
        //remove object
        objects.erase(other.id);
        emit dataChanged();
        return one;
    }

    void clearObjectSelection() {
        for(auto & obj : selectedObjects) {
            obj.get().selected = false;
        }
        selectedObjects.clear();
    }

signals:
    void dataChanged();

public slots:
    void mergeSelectedObjects() {
        for(auto iter = selectedObjects.rbegin(); iter != selectedObjects.rend() - 1; ++iter) {
            merge(selectedObjects.front().get(), std::move(iter->get()));
        }
    }
};

#endif // SEGEMENTATION_H
