#ifndef SEGEMENTATION_H
#define SEGEMENTATION_H

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <istream>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <time.h>

#include <QDebug>
#include <QString>
#include <QObject>
#include <QStandardPaths>

class Segmentation : public QObject {
Q_OBJECT
    friend void segmentationColorPicking(const int, const int, const int);
    friend class SegmentationObjectModel;
    friend class SegmentationTab;
    friend class Viewer;

    class Object;
    class SubObject {
        friend void segmentationColorPicking(const int, const int, const int);
        friend class Object;
        friend class Segmentation;
        friend class Viewer;
        std::vector<std::reference_wrapper<Object>> objects;
    public:
        uint64_t id;
        SubObject(const uint64_t id = 0) : id(id) {}
        SubObject(SubObject &&) = default;
        SubObject(const SubObject &) = delete;
        bool selected() {
            for(const auto & obj : objects) {
                if(obj.get().selected) {
                    return true;
                }
            }
            return false;
        }
        void unmerge() {
            for (auto & obj : objects) {
                std::remove_if(std::begin(obj.get().subobjects), std::end(obj.get().subobjects), [&](const SubObject & subobj){
                    return subobj.id == this->id;
                });
            }
        }
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
        Object(uint64_t id) : id(id), selected(false) {}
        Object(Object &&) = delete;
        Object(const Object &) = delete;
        Object(SubObject & initialVolume) : id(initialVolume.id), selected(false) {
            initialVolume.objects.emplace_back(*this);//register parent
            subobjects.emplace_back(initialVolume);//add child
        }
        bool operator==(const Object & other) {
            return id == other.id;
        }
        void addSubObject(SubObject & sub) {
            subobjects.emplace_back(sub);
            sub.objects.emplace_back(*this);
        }
    };

    std::unordered_map<uint64_t, SubObject> subobjects;
    std::unordered_map<uint64_t, Object> objects;
    std::vector<std::reference_wrapper<Object>> selectedObjects;
    bool renderAllObjs; // show all segmentations as opposed to only a selected one

    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> subobjectColor(const uint64_t subObjectID) const {
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
    bool segmentationMode;
    QString filename;

    Segmentation() : renderAllObjs(true), segmentationMode(true) {}

    bool hasObjects() {
        return !this->objects.empty();
    }

    void setDefaultFilename() {
        // Generate a default file name based on date and time.
        auto currentTime = time(nullptr);
        auto localTime = localtime(&currentTime);
        if(localTime->tm_year >= 100) {
            localTime->tm_year -= 100;
        }
        filename = QString(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
                + "/segmentationFiles/segmentation-%1%2%3-%4%5.000.mrg")
                //value, right aligned padded to width 2, base 10, filled with '0'
                .arg(localTime->tm_mday, 2, 10, QLatin1Char('0'))
                .arg(localTime->tm_mon + 1, 2, 10, QLatin1Char('0'))
                .arg(localTime->tm_year, 2, 10, QLatin1Char('0'))
                .arg(localTime->tm_hour, 2, 10, QLatin1Char('0'))
                .arg(localTime->tm_min, 2, 10, QLatin1Char('0'));
    }

    Object &addObject(const uint64_t id) {
        // emplace returns pair of iterator (first) and bool (second)
        Object &obj = objects.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id)).first->second;
        return obj;
    }

    void createObjectFromSubobjectId(const uint64_t id) {
        if (subobjects.find(id) == std::end(subobjects)) {//insert only if not already present
            auto & newKeyValuePair = *subobjects.emplace(id, id).first;//first is iterator to the newly inserted key-value pair
            objects.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(newKeyValuePair.second));//create object from supervoxel
        }
        emit dataChanged();
    }

    /**
     * @brief newSubObject create new subobject and add to given object
     */
    void newSubObject(Object & obj, uint64_t subObjID) {
        auto & newSubObjIt = *subobjects.emplace(subObjID, subObjID).first;
        obj.addSubObject(newSubObjIt.second);
    }

    void parseCube(const std::string & path) {
        std::ifstream cubeFile(path, std::ios_base::binary);
        std::vector<uint64_t> cube(128 * 128 * 128);
        cubeFile.read(reinterpret_cast<char * const>(cube.data()), cube.size() * sizeof(cube[0]));
        cube = decltype(cube)(std::begin(cube), std::unique(std::begin(cube), std::end(cube)));
        for (const auto & id : cube) {
            createObjectFromSubobjectId(id);
        }
        emit dataChanged();
    }
    void parseIdList(const std::string & path) {
        static std::unordered_map<std::string, bool> idFiles;
        if (idFiles.find(path) != std::end(idFiles)) {
            return;//file already loaded
        }
        idFiles.emplace(path, true);

        std::ifstream idFile(path, std::ios_base::binary);

        std::size_t idCounter = 0;
        uint64_t readValue;
        while (idFile.read(reinterpret_cast<char * const>(&readValue), sizeof(readValue))) {
            ++idCounter;
            createObjectFromSubobjectId(readValue);
        }
        emit dataChanged();
    }
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> subobjectColorUnique(const uint64_t subObjectID) const {
        const uint8_t red   =  subObjectID        & 0xFF;
        const uint8_t green = (subObjectID >> 8)  & 0xFF;
        const uint8_t blue  = (subObjectID >> 16) & 0xFF;

        return std::make_tuple(red, green, blue, 255);
    }
    uint64_t subobjectFromUniqueColor(std::tuple<uint8_t, uint8_t, uint8_t> color) const {
        return std::get<0>(color) + (std::get<1>(color) << 8) + (std::get<2>(color) << 16);
    }
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> objectColorFromSubobject(const uint64_t subObjectID) const {
        if (subObjectID == 0) {
            return std::make_tuple(0, 0, 0, 0);
        }
        const auto subobjectIt = subobjects.find(subObjectID);
        if (subobjectIt == std::end(subobjects)) {
            return subobjectColor(subObjectID);
        }
        const auto & subobject = subobjectIt->second;
        if (subobject.objects.front().get().id == 0) {
            return std::make_tuple(0, 0, 0, 0);
        }

        uint8_t   red = 0;
        uint8_t green = 0;
        uint8_t  blue = 0;
        uint8_t currAlpha = alpha;
        bool selected = false;
        const auto objectCount = subobject.objects.size();
        for (const auto & object : subobject.objects) {
            if(object.get().selected) {
                selected = true;
            }
            const auto objectID = object.get().id;
            red   += overlayColorMap[0][objectID % 256] / objectCount;
            green += overlayColorMap[1][objectID % 256] / objectCount;
            blue  += overlayColorMap[2][objectID % 256] / objectCount;
        }
        if(renderAllObjs == false && selected == false) {
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
    void unmerge(SubObject & subobject) {
        subobject.unmerge();
        subobjects.erase(subobject.id);
    }
    void clearObjectSelection() {
        for(auto & obj : selectedObjects) {
            obj.get().selected = false;
        }
        selectedObjects.clear();
    }
    void selectObject(const uint64_t objectId) {
        auto iter = objects.find(objectId);
        if (iter != std::end(objects)) {
            iter->second.selected = true;
            selectedObjects.push_back(iter->second);
        }
    }
    std::size_t selectedObjectsCount() {
        return selectedObjects.size();
    }

    void saveMergelist(QString toFile=Segmentation::singleton().filename) {
        std::string objString;
        for(const auto & obj : objects) {
            objString += std::to_string(obj.first) + " ";
            for(const auto & subObj : obj.second.subobjects) {
                objString += std::to_string(subObj.get().id) + " ";
            }
            objString.pop_back();
            objString += "\n";
        }
        objString.pop_back();
        std::ofstream file(toFile.toStdString());
        file << objString;
        file.close();
    }

    void loadMergelist(std::string fileName) {
        std::ifstream file;
        file.open(fileName, std::ifstream::in);
        if(file.good()) {
            while(file.eof() == false) {
                std::string line;
                std::getline(file, line);

                std::stringstream lineStrm(line);
                std::vector<std::string> elems;
                std::string elem;
                while(std::getline(lineStrm, elem, ' ')) {
                    elems.push_back(elem);
                }
                if(elems.size() == 0) {
                    continue;
                }
                uint64_t objID;
                std::stringstream strstream(elems[0]);
                strstream >> objID;
                if(strstream.fail()) {
                    qDebug("invalid entry");
                    continue;
                }
                elems.erase(elems.begin());
                Object &obj = addObject(objID);
                for(const std::string & elem : elems) {
                    uint64_t id;
                    std::stringstream subObjStrStream(elem);
                    subObjStrStream >> id;
                    if(subObjStrStream.fail()) {
                        qDebug("invalid subobj entry");
                        continue;
                    }
                    newSubObject(obj, id);
                }
            }
        }
        else {
            qDebug("File not found!");
        }
        file.close();
        emit dataChanged();
    }

signals:
    void dataChanged();

public slots:
    void mergeSelectedObjects() {
        while (selectedObjects.size() > 1) {
            auto & obj = selectedObjects.back().get();
            obj.selected = false;
            selectedObjects.pop_back();
            if (obj.id != 0) {//don’t merge void into another object
                merge(selectedObjects.front().get(), std::move(obj));
            }
        }
        selectedObjects.back().get().selected = false;
        selectedObjects.pop_back();
        emit dataChanged();
    }
};

#endif // SEGEMENTATION_H
