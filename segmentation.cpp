#include "segmentation.h"

uint64_t Segmentation::highestObjectId = 1;

Segmentation::Object::Object(Segmentation::SubObject & initialVolume)
    : id(initialVolume.id), immutable(false), comment("") {
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const uint64_t id, const bool & immutable, Segmentation::SubObject & initialVolume)
    : id(id), immutable(immutable), comment("") {
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const uint64_t id, const bool & immutable,
                             std::vector<std::reference_wrapper<SubObject>> initialVolumes)
    : id(id), immutable(immutable), comment("") {
    for (auto & elem : initialVolumes) {
        addExistingSubObject(elem);
    }
}

Segmentation::Object::Object(const uint64_t id, const Object & first,
                             const Segmentation::Object & second)
    : id(id), immutable(false), comment("") {
    merge(first);
    merge(second);
}

bool Segmentation::Object::operator==(const Segmentation::Object & other) const {
    return id == other.id;
}

void Segmentation::Object::addExistingSubObject(Segmentation::SubObject & sub) {
    subobjects.emplace_back(sub);//add child
    sub.objects.emplace_back(*this);//register parent
}

Segmentation::Object & Segmentation::Object::merge(const Segmentation::Object & other) {
    for (auto & elem : other.subobjects) {//add parent
        auto & objects = elem.get().objects;
        //don’t insert twice
        if (std::find(std::begin(objects), std::end(objects), std::cref(*this)) == std::end(objects)) {
            objects.push_back(*this);
        }
    }

    decltype(subobjects) tmp(std::begin(subobjects), std::end(subobjects));
    tmp.insert(std::end(tmp), std::begin(other.subobjects), std::end(other.subobjects));
    std::sort(std::begin(tmp), std::end(tmp)), std::end(tmp);
    tmp.erase(std::unique(std::begin(tmp), std::end(tmp)), std::end(tmp));
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
    return objects.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(id, one, other)).first->second;
}

Segmentation::Object & Segmentation::merge(Segmentation::Object & one, Segmentation::Object & other) {
    one.moveFrom(other);
    //remove object
    objects.erase(other.id);
    emit dataChanged();
    return one;
}

void Segmentation::loadOverlayLutFromFile(const std::string & filename) {
    std::ifstream overlayLutFile(filename, std::ios_base::binary);
    if (overlayLutFile) {
        std::vector<char> buffer(std::istreambuf_iterator<char>(overlayLutFile), std::istreambuf_iterator<char>{});//consume whole file

        const auto expectedSize = 768 * sizeof(buffer[0]);
        if (buffer.size() == expectedSize) {
            std::move(std::begin(buffer), std::end(buffer), &Segmentation::singleton().overlayColorMap[0][0]);
            qDebug() << "sucessfully loaded »stdOverlay.lut«";
        } else {
            qDebug() << "stdOverlay.lut corrupted: expected" << expectedSize << "bytes got" << buffer.size() << "bytes";
        }
    }
}

bool Segmentation::hasObjects() const {
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
    obj.addExistingSubObject(newSubObj);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::colorUniqueFromId(const uint64_t subObjectID) const {
    const uint8_t red   =  subObjectID        & 0xFF;
    const uint8_t green = (subObjectID >> 8)  & 0xFF;
    const uint8_t blue  = (subObjectID >> 16) & 0xFF;

    return std::make_tuple(red, green, blue, 255);
}

uint64_t Segmentation::subobjectIdFromUniqueColor(std::tuple<uint8_t, uint8_t, uint8_t> color) const {
    return std::get<0>(color) + (std::get<1>(color) << 8) + (std::get<2>(color) << 16);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::colorOfSelectedObject(const SubObject & subobject) const {
    bool multipleSelectedObjects = std::count_if(std::begin(subobject.objects), std::end(subobject.objects), [](const Segmentation::Object & object){
        return object.selected;
    }) > 1;
    if (multipleSelectedObjects) {
        return std::make_tuple(255, 0, 0, alpha);
    }

    auto & object = std::find_if(std::begin(subobject.objects), std::end(subobject.objects), [](const Segmentation::Object & object){
        return object.selected;
    })->get();

    const uint8_t red   = overlayColorMap[0][object.id % 256];
    const uint8_t green = overlayColorMap[1][object.id % 256];
    const uint8_t blue  = overlayColorMap[2][object.id % 256];
    return std::make_tuple(red, green, blue, alpha);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::colorObjectFromId(const uint64_t subObjectID) const {
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
    if (subobject.selected) {
        return colorOfSelectedObject(subobject);
    } else if (!renderAllObjs) {
        return std::make_tuple(0, 0, 0, 0);
    }

    const auto & object = largestObjectContainingSubobject(subobject);
    const uint8_t red   = overlayColorMap[0][object.id % 256];
    const uint8_t green = overlayColorMap[1][object.id % 256];
    const uint8_t blue  = overlayColorMap[2][object.id % 256];
    return std::make_tuple(red, green, blue, alpha);
}

bool Segmentation::subobjectExists(const uint64_t & subobjectId) const {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects);
}

Segmentation::SubObject & Segmentation::subobjectFromId(const uint64_t & subobjectId) {
    //check if subobject exists
    auto it = subobjects.find(subobjectId);
    if (it == std::end(subobjects)) {//create an object for the selected subobject
        const auto id = ++highestObjectId;
        createObject(id, subobjectId);
        it = subobjects.find(subobjectId);
        emit dataChanged();
    }
    return it->second;
}

Segmentation::Object & Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    return std::max_element(std::begin(subobject.objects), std::end(subobject.objects)
        , [](const Segmentation::Object & lhs, const Segmentation::Object & rhs){
            return (lhs.immutable && !rhs.immutable) || (lhs.immutable == rhs.immutable && lhs.subobjects.size() < rhs.subobjects.size());
        })->get();
}

Segmentation::Object & Segmentation::largestImmutableObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    return std::max_element(std::begin(subobject.objects), std::end(subobject.objects)
        , [](const Segmentation::Object & lhs, const Segmentation::Object & rhs){
            return (!lhs.immutable && rhs.immutable) || (lhs.immutable == rhs.immutable && lhs.subobjects.size() < rhs.subobjects.size());
        })->get();
}

void Segmentation::touchObjects(const uint64_t subobject_id) {
    touched_subobject_id = subobject_id;
    emit dataChanged();
}

void Segmentation::untouchObjects() {
    touched_subobject_id = 0;
    emit dataChanged();
}

std::vector<std::reference_wrapper<Segmentation::Object>> Segmentation::touchedObjects() {
    auto it = subobjects.find(touched_subobject_id);
    if (it != std::end(subobjects)) {
        return it->second.objects;
    } else {
        return {};
    }
}

bool Segmentation::isSelected(const SubObject & rhs) const{
    return rhs.selected;
}

bool Segmentation::isSelected(const Object & rhs) const {
    return rhs.selected;
}

void Segmentation::clearObjectSelection() {
    bool blockState = this->signalsBlocked();
    this->blockSignals(true);
    while (!selectedObjects.empty()) {
        unselectObject(selectedObjects.front());
    }
    this->blockSignals(blockState);
    emit dataChanged();
}

void Segmentation::selectObject(Object & object) {
    if(object.selected) {
        return;
    }
    object.selected = true;
    for (auto & subobj : object.subobjects) {
        subobj.get().selected = true;
    }
    selectedObjects.push_back(object);
    emit dataChanged();
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
    selectedObjects.erase(std::remove(std::begin(selectedObjects), std::end(selectedObjects), std::cref(object)));
    emit dataChanged();
}

void Segmentation::unmergeObject(Segmentation::Object & object, Segmentation::Object & other) {
    auto & otherSubobjects = other.subobjects;

    std::sort(std::begin(object.subobjects), std::end(object.subobjects));
    std::sort(std::begin(otherSubobjects), std::end(otherSubobjects));

    decltype(object.subobjects) tmp;
    std::set_difference(std::begin(object.subobjects), std::end(object.subobjects), std::begin(otherSubobjects), std::end(otherSubobjects), std::back_inserter(tmp));
    if (!tmp.empty()) {//only unmerge if subobjects remain
        if (object.immutable) {
            unselectObject(object);
            auto objectId = ++highestObjectId;
            auto & newObject = objects.emplace(std::piecewise_construct, std::forward_as_tuple(objectId), std::forward_as_tuple(objectId, false, tmp)).first->second;
            selectObject(newObject);
        } else {
            unselectObject(object);
            for (auto & elem : otherSubobjects) {
                auto & objects = elem.get().objects;
                objects.erase(std::remove(std::begin(objects), std::end(objects), std::cref(object)));//remove parent
            }
            std::swap(object.subobjects, tmp);
            selectObject(object);
        }
        emit dataChanged();
    }
}

void Segmentation::selectObjectFromSubObject(Segmentation::SubObject & subobject) {
    const auto & other = std::find_if(std::begin(subobject.objects), std::end(subobject.objects)
        , [&](const Segmentation::Object & elem){
            return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobject.id;
        });
    if (other == std::end(subobject.objects)) {
        auto objectId = ++highestObjectId;
        auto & newObject = objects.emplace(std::piecewise_construct, std::forward_as_tuple(objectId), std::forward_as_tuple(objectId, false, subobject)).first->second;
        selectObject(newObject);
    } else {
        selectObject(*other);
    }
}

void Segmentation::selectObject(const uint64_t & objectId) {
    auto iter = objects.find(objectId);
    if (iter != std::end(objects)) {
        selectObject(iter->second);
    }
}

std::size_t Segmentation::selectedObjectsCount() const {
    return selectedObjects.size();
}

void Segmentation::saveMergelist(const QString & toFile) {
    std::ofstream file(toFile.toStdString());
    for (const auto & obj : objects) {
        file << obj.first << " " << obj.second.immutable;
        for (const auto & subObj : obj.second.subobjects) {
            file << " " << subObj.get().id;
        }
        file << std::endl << obj.second.category.toStdString() << std::endl << obj.second.comment.toStdString()
             << std::endl;
    }
}

void Segmentation::loadMergelist(const std::string & fileName) {
    std::ifstream file(fileName);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        Object *obj;
        if(lineCount == 0) {
            std::istringstream lineIss(line);
            uint64_t objID;
            bool immutable;
            uint64_t initialVolume;

            if (lineIss >> objID && lineIss >> immutable && lineIss >> initialVolume) {
                obj = &createObject(objID, initialVolume, immutable);
                while (lineIss >> objID) {
                    newSubObject(*obj, objID);
                }
                lineCount++;
            }
            else {
                qDebug() << "loadMergelist fail";
            }
        }
        else if(lineCount == 1) {
            obj->category = QString::fromStdString(line);
            lineCount++;
        }
        else if(lineCount == 2) {
            obj->comment = QString::fromStdString(line);
            lineCount = 0;
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

void Segmentation::unmergeSelectedObjects(Segmentation::SubObject & subobjectToUnmerge) {
    const auto & otherIt = std::find_if(std::begin(subobjectToUnmerge.objects), std::end(subobjectToUnmerge.objects)
        , [&](const Segmentation::Object & elem){
            return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobjectToUnmerge.id;
        });
    //create object to unmerge if there’s none except the currently selected one
    if (otherIt == std::end(subobjectToUnmerge.objects)) {
        auto objectId = ++highestObjectId;
        auto & other = objects.emplace(std::piecewise_construct, std::forward_as_tuple(objectId), std::forward_as_tuple(objectId, false, subobjectToUnmerge)).first->second;
        unmergeSelectedObjects(other);
    } else {
        unmergeSelectedObjects(otherIt->get());
    }
}

void Segmentation::unmergeSelectedObjects(Segmentation::Object & objectToUnmerge) {
    for (auto & obj : selectedObjects) {
        unmergeObject(obj, objectToUnmerge);
    }
    emit dataChanged();
}
