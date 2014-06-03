#include "segmentation.h"

uint64_t Segmentation::highestObjectId = 1;

Segmentation::Object::Object(Segmentation::SubObject & initialVolume) : id(initialVolume.id), immutable(false), selected(false) {
    addSubObject(initialVolume);
}

Segmentation::Object::Object(const uint64_t id, const bool & immutable, Segmentation::SubObject & initialVolume) : id(id), immutable(immutable), selected(false) {
    addSubObject(initialVolume);
}

Segmentation::Object::Object(const uint64_t id, const Segmentation::Object & first, const Segmentation::Object & second) : id(id), immutable(false), selected(false) {
    merge(first);
    merge(second);
}

bool Segmentation::Object::operator==(const Segmentation::Object & other) const {
    return id == other.id;
}

void Segmentation::Object::addSubObject(Segmentation::SubObject & sub) {
    subobjects.emplace_back(sub);//add child
    sub.objects.emplace_back(*this);//register parent
}

Segmentation::Object & Segmentation::Object::merge(const Segmentation::Object & other) {
    auto otherSubobjects = other.subobjects;//copy
    for (auto & elem : otherSubobjects) {//add parent
        auto & objects = elem.get().objects;
        //don’t insert twice
        if (std::find(std::begin(objects), std::end(objects), std::cref(*this)) == std::end(objects)) {
            objects.push_back(*this);
        }
    }

    std::sort(std::begin(subobjects), std::end(subobjects));
    std::sort(std::begin(otherSubobjects), std::end(otherSubobjects));

    decltype(subobjects) tmp;
    std::merge(std::begin(subobjects), std::end(subobjects), std::begin(otherSubobjects), std::end(otherSubobjects), std::back_inserter(tmp));
    std::swap(subobjects, tmp);
    return *this;
}

Segmentation::Object & Segmentation::Object::moveFrom(Segmentation::Object & other) {
    for (auto & elem : other.subobjects) {//remove parent
        auto & objects = elem.get().objects;
        objects.erase(std::remove(std::begin(objects), std::end(objects), std::cref(other)));
    }
    merge(other);
    return *this;
}

Segmentation & Segmentation::singleton() {
    static Segmentation segmentation;
    return segmentation;
}

Segmentation::Segmentation() : renderAllObjs(true), segmentationMode(true) {}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::subobjectColor(const uint64_t subObjectID) const {
    const uint8_t red   = overlayColorMap[0][subObjectID % 256];
    const uint8_t green = overlayColorMap[1][subObjectID % 256];
    const uint8_t blue  = overlayColorMap[2][subObjectID % 256];
    return std::make_tuple(red, green, blue, alpha);
}

Segmentation::Object & Segmentation::const_merge(Segmentation::Object & one, Segmentation::Object & other) {
    const auto id = ++highestObjectId;
    return objects.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, one, other)).first->second;//create object from supervoxel
}

Segmentation::Object & Segmentation::merge(Segmentation::Object & one, Segmentation::Object & other) {
    one.moveFrom(other);
    //remove object
    objects.erase(other.id);
    emit dataChanged();
    return one;
}

bool Segmentation::hasObjects() {
    return !this->objects.empty();
}

void Segmentation::setDefaultFilename() {
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

Segmentation::Object & Segmentation::createObject(const uint64_t initialSubobjectId) {
    auto & obj = createObject(initialSubobjectId, initialSubobjectId);
    emit dataChanged();
    return obj;
}

Segmentation::Object & Segmentation::createObject(const uint64_t objectId, const uint64_t initialSubobjectId, const bool & immutable) {
    const auto & objectIt = objects.find(objectId);
    if (objectIt == std::end(objects)) {
        highestObjectId = objectId > highestObjectId ? objectId : highestObjectId;
        //first is iterator to the newly inserted key-value pair
        auto & subobject = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(initialSubobjectId), std::forward_as_tuple(initialSubobjectId)).first->second;
        return objects.emplace(std::piecewise_construct, std::forward_as_tuple(objectId), std::forward_as_tuple(objectId, immutable, subobject)).first->second;//create object from supervoxel
    } else {
        qDebug() << "tried to create object with id" << objectId << "which already exists";
        return objectIt->second;
    }
}

void Segmentation::removeObject(Object & object) {
    unselectObject(object);
    selectedObjects.erase(std::remove(std::begin(selectedObjects), std::end(selectedObjects), std::cref(object)));
    for (auto & elem : object.subobjects) {
        auto & subobject = elem.get();
        subobject.objects.erase(std::remove(std::begin(subobject.objects), std::end(subobject.objects), std::cref(object)));
        if (subobject.objects.empty()) {
            subobjects.erase(subobject.id);
        }
    }
    objects.erase(object.id);
}

void Segmentation::newSubObject(Object & obj, uint64_t subObjID) {
    auto & newSubObj = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(subObjID), std::forward_as_tuple(subObjID)).first->second;
    obj.addSubObject(newSubObj);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::subobjectColorUniqueFromId(const uint64_t subObjectID) const {
    const uint8_t red   =  subObjectID        & 0xFF;
    const uint8_t green = (subObjectID >> 8)  & 0xFF;
    const uint8_t blue  = (subObjectID >> 16) & 0xFF;

    return std::make_tuple(red, green, blue, 255);
}

uint64_t Segmentation::subobjectIdFromUniqueColor(std::tuple<uint8_t, uint8_t, uint8_t> color) const {
    return std::get<0>(color) + (std::get<1>(color) << 8) + (std::get<2>(color) << 16);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::objectColorFromSubobject(const uint64_t subObjectID) const {
    if (subObjectID == 0) {
        return std::make_tuple(0, 0, 0, 0);
    }
    const auto subobjectIt = subobjects.find(subObjectID);
    if (subobjectIt == std::end(subobjects)) {
        if (renderAllObjs || selectedObjects.empty()) {
            return subobjectColor(subObjectID);
        } else {
            return std::make_tuple(0, 0, 0, 0);
        }
    }
    const auto & subobject = subobjectIt->second;
    if (subobject.objects.front().get().id == 0) {
        return std::make_tuple(0, 0, 0, 0);
    }

    const auto & object = largestObjectContainingSubobject(subobject);
    const uint8_t red   = overlayColorMap[0][object.id % 256];
    const uint8_t green = overlayColorMap[1][object.id % 256];
    const uint8_t blue  = overlayColorMap[2][object.id % 256];
    uint8_t currAlpha = alpha;
    if (renderAllObjs == false && object.selected == false) {
        currAlpha = 0;
    }
    return std::make_tuple(red, green, blue, currAlpha);
}

bool Segmentation::subobjectExists(const uint64_t & subobjectId) {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects);
}

Segmentation::SubObject & Segmentation::subobjectFromId(const uint64_t & subobjectId) {
    //check if subobject exists
    auto it = subobjects.find(subobjectId);
    if (it == std::end(subobjects)) {//create an object for the selected subobject
        createObject(subobjectId);
        it = subobjects.find(subobjectId);
    }
    return it->second;
}

Segmentation::Object & Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    return std::max_element(std::begin(subobject.objects), std::end(subobject.objects)
        , [](const Segmentation::Object & lhs, const Segmentation::Object & rhs){
            return lhs.subobjects.size() < rhs.subobjects.size();
        })->get();
}

void Segmentation::touchObjects(const Segmentation::SubObject & subobject)  {
    touchedObjects.assign(subobject.objects.begin(), subobject.objects.end());
}

bool Segmentation::isSelected(const SubObject & rhs) {
    return rhs.selected;
}

bool Segmentation::isSelected(const Object & rhs) {
    return rhs.selected;
}

void Segmentation::clearObjectSelection() {
    for (auto & obj : selectedObjects) {
        unselectObject(obj.get());
    }
    selectedObjects.clear();
}

void Segmentation::selectObject(Object & object) {
    object.selected = true;
    for (auto & subobj : object.subobjects) {
        subobj.get().selected = true;
    }
    selectedObjects.push_back(object);
    emit selectionChanged();
}

void Segmentation::unselectObject(Object & object) {
    object.selected = false;
    for (auto & subobj : object.subobjects) {
        subobj.get().selected = false;
        for (const auto & obj : subobj.get().objects) {
            if (obj.get().selected) {
                subobj.get().selected = true;
                break;
            }
        }
    }
}

void Segmentation::unmerge(Segmentation::Object & object, Segmentation::SubObject & subobject) {
    if (subobject.objects.size() == 1) {//create object to unmerge if there’s none except the currently selected one
        auto objectId = ++highestObjectId;
        objects.emplace(std::piecewise_construct, std::forward_as_tuple(objectId), std::forward_as_tuple(objectId, false, subobject));
    }
    //find object with lowest subobject count
    const auto & other = std::min_element(std::begin(subobject.objects), std::end(subobject.objects)
        , [](const Segmentation::Object & lhs, const Segmentation::Object & rhs){
            return lhs.subobjects.size() < rhs.subobjects.size();
        })->get();
    auto otherSubobjects = other.subobjects;//copy
    for (auto & elem : otherSubobjects) {
        auto & objects = elem.get().objects;
        objects.erase(std::remove(std::begin(objects), std::end(objects), std::cref(object)));//remove parent
        //WARNING this could break if there’s another selected object
        elem.get().selected = false;//the removed content shall no longer be selected
    }

    std::sort(std::begin(object.subobjects), std::end(object.subobjects));
    std::sort(std::begin(otherSubobjects), std::end(otherSubobjects));

    decltype(object.subobjects) tmp;
    std::set_difference(std::begin(object.subobjects), std::end(object.subobjects), std::begin(otherSubobjects), std::end(otherSubobjects), std::back_inserter(tmp));
    std::swap(object.subobjects, tmp);
    emit dataChanged();
}

void Segmentation::selectObject(const uint64_t & objectId) {
    auto iter = objects.find(objectId);
    if (iter != std::end(objects)) {
        selectObject(iter->second);
    }
}

std::size_t Segmentation::selectedObjectsCount() {
    return selectedObjects.size();
}

void Segmentation::saveMergelist(const QString & toFile) {
    std::ofstream file(toFile.toStdString());
    for (const auto & obj : objects) {
        file << obj.first << " " << obj.second.immutable;
        for (const auto & subObj : obj.second.subobjects) {
            file << " " << subObj.get().id;
        }
        file << std::endl;
    }
}

void Segmentation::loadMergelist(const std::string & fileName) {
    std::ifstream file(fileName);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream lineIss(line);
        uint64_t objID;
        bool immutable;
        uint64_t initialVolume;

        if (lineIss >> objID && lineIss >> immutable && lineIss >> initialVolume) {
            auto & obj = createObject(objID, initialVolume, immutable);
            while (lineIss >> objID) {
                newSubObject(obj, objID);
            }
        } else {
            qDebug() << "loadMergelist fail";
        }
    }
    emit dataChanged();
}

void Segmentation::deleteSelectedObjects() {
    for (auto & elem : selectedObjects) {
        removeObject(elem.get());
    }
    emit dataChanged();
}

void Segmentation::mergeSelectedObjects() {
    while (selectedObjects.size() > 1) {
        auto & obj = selectedObjects.back().get();
        obj.selected = false;
        selectedObjects.pop_back();
        if (obj.id != 0) {//don’t merge void into another object
            auto & first = selectedObjects.front().get();
            if (first.immutable && obj.immutable) {
                auto & newObj = const_merge(first, obj);
                first.selected = false;
                newObj.selected = true;
                selectedObjects.front() = newObj;
            } else if (first.immutable) {
                obj.merge(first);
                first.selected = false;
                obj.selected = true;
                selectedObjects.front() = obj;
            } else if (obj.immutable) {
                first.merge(obj);
            } else {
                merge(selectedObjects.front().get(), obj);
            }
        }
    }
    emit dataChanged();
}
