#include "segmentation.h"

#include <QTextStream>

#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>
#include <utility>

uint64_t Segmentation::Object::highestId = -1;

Segmentation::Object::Object(Segmentation::SubObject & initialVolume)
    : immutable(false)
{
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const bool & immutable, Segmentation::SubObject & initialVolume)
    : immutable(immutable)
{
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const bool & immutable, std::vector<std::reference_wrapper<SubObject>> initialVolumes)
    : immutable(immutable)
{
    for (auto & elem : initialVolumes) {
        addExistingSubObject(elem);
    }
    std::sort(std::begin(subobjects), std::end(subobjects));
}

Segmentation::Object::Object(Object & first, Object & second)
    : immutable(false), selected{true}//merge is selected
{
    subobjects.reserve(first.subobjects.size() + second.subobjects.size());
    merge(first);
    merge(second);
    subobjects.shrink_to_fit();
}

bool Segmentation::Object::operator==(const Segmentation::Object & other) const {
    return id == other.id;
}

void Segmentation::Object::addExistingSubObject(Segmentation::SubObject & sub) {
    subobjects.emplace_back(sub);//add child
    auto objectPosIt = std::lower_bound(std::begin(sub.objects), std::end(sub.objects), this->id);
    sub.objects.emplace(objectPosIt, this->id);//register parent
}

Segmentation::Object & Segmentation::Object::merge(Segmentation::Object & other) {
    for (auto & elem : other.subobjects) {//add parent
        auto & objects = elem.get().objects;
        elem.get().selectedObjectsCount = 1;
        //don’t insert twice
        auto posIt = std::lower_bound(std::begin(objects), std::end(objects), this->id);
        if (posIt == std::end(objects) || *posIt != this->id) {
            objects.emplace(posIt, this->id);
        }
    }
    decltype(subobjects) tmp;
    std::merge(std::begin(subobjects), std::end(subobjects), std::begin(other.subobjects), std::end(other.subobjects), std::back_inserter(tmp));
    std::swap(subobjects, tmp);
    subobjects.erase(std::unique(std::begin(subobjects), std::end(subobjects)), std::end(subobjects));
    return *this;
}

Segmentation & Segmentation::singleton() {
    static Segmentation segmentation;
    return segmentation;
}

Segmentation::Segmentation() : renderAllObjs(true), segmentationMode(true) {}

void Segmentation::clear() {
    selectedObjects.clear();
    objects.clear();
    Object::highestId = -1;
    subobjects.clear();
    touched_subobject_id = 0;

    emit resetData();
    emit resetSelection();
    emit resetTouchedObjects();
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::subobjectColor(const uint64_t subObjectID) const {
    const uint8_t red   = overlayColorMap[0][subObjectID % 256];
    const uint8_t green = overlayColorMap[1][subObjectID % 256];
    const uint8_t blue  = overlayColorMap[2][subObjectID % 256];
    return std::make_tuple(red, green, blue, alpha);
}

Segmentation::Object & Segmentation::const_merge(Segmentation::Object & one, Segmentation::Object & other) {
    emit beforeAppendRow();
    objects.emplace_back(one, other);
    emit appendedRow();
    return objects.back();
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

Segmentation::Object & Segmentation::createObject(const uint64_t initialSubobjectId, const bool & immutable) {
    //first is iterator to the newly inserted key-value pair or the already existing value
    auto subobjectIt = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(initialSubobjectId), std::forward_as_tuple(initialSubobjectId)).first;
    auto & subobject = subobjectIt->second;
    emit beforeAppendRow();
    objects.emplace_back(immutable, subobject);//create object from supervoxel
    emit appendedRow();
    return objects.back();
}

void Segmentation::removeObject(Object & object) {
    unselectObject(object);
    for (auto & elem : object.subobjects) {
        auto & subobject = elem.get();
        subobject.objects.erase(std::remove(std::begin(subobject.objects), std::end(subobject.objects), object.id));
        if (subobject.objects.empty()) {
            subobjects.erase(subobject.id);
        }
    }
    //swap with last, so no intermediate rows need to be deleted
    if (objects.size() > 1) {
        //replace object id in subobjects
        for (auto & elem : objects.back().subobjects) {
            auto & subobject = elem.get();
            std::replace(std::begin(subobject.objects), std::end(subobject.objects), objects.back().id, object.id);
        }
        //replace object id in selected objects
        if (selectedObjects.find(objects.back().id) != std::end(selectedObjects)) {
            selectedObjects.erase(objects.back().id);
            selectedObjects.emplace(object.id);
        }
        std::swap(object.id, objects.back().id);
        std::swap(object, objects.back());
    }
    emit beforeRemoveRow();
    objects.pop_back();
    emit removedRow();
    --Object::highestId;
}

void Segmentation::newSubObject(Object & obj, uint64_t subObjID) {
    auto subobjectIt = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(subObjID), std::forward_as_tuple(subObjID)).first;
    obj.addExistingSubObject(subobjectIt->second);
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
    if (subobject.selectedObjectsCount > 1) {
        return std::make_tuple(255, 0, 0, alpha);//mark overlapping objects in red
    }
    const auto objectId = *std::find_if(std::begin(subobject.objects), std::end(subobject.objects), [this](const uint64_t id){
        return objects[id].selected;
    });
    const auto & object = objects[objectId];
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
    if (subobject.selectedObjectsCount != 0) {
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
    auto it = subobjects.find(subobjectId);//check if subobject exists
    if (it == std::end(subobjects)) {
        createObject(subobjectId);//create an object for the selected subobject
        it = subobjects.find(subobjectId);
    }
    return it->second;
}

bool Segmentation::objectOrder(const uint64_t & lhsId, const uint64_t & rhsId) const {
    const auto & lhs = objects[lhsId];
    const auto & rhs = objects[rhsId];
    //operator< substitute, prefer immutable objects and choose the smallest
    return (lhs.immutable && !rhs.immutable) || (lhs.immutable == rhs.immutable && lhs.subobjects.size() < rhs.subobjects.size());
}

Segmentation::Object & Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) {
    const auto & segmentationConstRef = *this;
    return const_cast<Segmentation::Object&>(segmentationConstRef.largestObjectContainingSubobject(subobject));
}
const Segmentation::Object & Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    //same comparitor for both functions, it seems to work as it is, so i don’t waste my head now to find out why
    //there may have been some reasoning… (at first glance it seems to restrictive for the largest object)
    auto comparitor = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectId = *std::max_element(std::begin(subobject.objects), std::end(subobject.objects), comparitor);
    return objects[objectId];
}

Segmentation::Object & Segmentation::smallestImmutableObjectContainingSubobject(const Segmentation::SubObject & subobject) {
    const auto & segmentationConstRef = *this;
    return const_cast<Segmentation::Object&>(segmentationConstRef.smallestImmutableObjectContainingSubobject(subobject));
}
const Segmentation::Object & Segmentation::smallestImmutableObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    auto comparitor = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectId = *std::min_element(std::begin(subobject.objects), std::end(subobject.objects), comparitor);
    return objects[objectId];
}

void Segmentation::touchObjects(const uint64_t subobject_id) {
    touched_subobject_id = subobject_id;
    emit resetTouchedObjects();
}

void Segmentation::untouchObjects() {
    touched_subobject_id = 0;
    emit resetTouchedObjects();
}

std::vector<std::reference_wrapper<Segmentation::Object>> Segmentation::touchedObjects() {
    auto it = subobjects.find(touched_subobject_id);
    if (it != std::end(subobjects)) {
        std::vector<std::reference_wrapper<Segmentation::Object>> vec;
        for (const auto & id : it->second.objects) {
            vec.emplace_back(objects[id]);
        }
        return vec;
    } else {
        return {};
    }
}

bool Segmentation::isSelected(const SubObject & rhs) const{
    return rhs.selectedObjectsCount != 0;
}

bool Segmentation::isSelected(const Object & rhs) const {
    return rhs.selected;
}

void Segmentation::clearObjectSelection() {
    while (!selectedObjects.empty()) {
        unselectObject(*std::begin(selectedObjects));
    }
}

void Segmentation::selectObject(Object & object) {
    if (object.selected) {
        return;
    }
    object.selected = true;
    for (auto & subobj : object.subobjects) {
        ++subobj.get().selectedObjectsCount;
    }
    selectedObjects.emplace(object.id);
    emit changedRow(object.id);
}

void Segmentation::unselectObject(const uint64_t & objectId) {
    if (objectId < objects.size()) {
        unselectObject(objects[objectId]);
    }
}

void Segmentation::unselectObject(Object & object) {
    if (!object.selected) {
        return;
    }
    object.selected = false;
    for (auto & subobj : object.subobjects) {
        --subobj.get().selectedObjectsCount;
    }
    selectedObjects.erase(object.id);
    emit changedRow(object.id);
}

void Segmentation::unmergeObject(Segmentation::Object & object, Segmentation::Object & other) {
    auto & otherSubobjects = other.subobjects;
    decltype(object.subobjects) tmp;
    std::set_difference(std::begin(object.subobjects), std::end(object.subobjects), std::begin(otherSubobjects), std::end(otherSubobjects), std::back_inserter(tmp));
    if (!tmp.empty()) {//only unmerge if subobjects remain
        if (object.immutable) {
            unselectObject(object);
            emit beforeAppendRow();
            objects.emplace_back(false, tmp);
            emit appendedRow();
            selectObject(objects.back());
        } else {
            unselectObject(object);
            for (auto & elem : otherSubobjects) {
                auto & objects = elem.get().objects;
                objects.erase(std::remove(std::begin(objects), std::end(objects), object.id));//remove parent
            }
            std::swap(object.subobjects, tmp);
            selectObject(object);
            emit changedRow(object.id);
        }
    }
}

void Segmentation::selectObjectFromSubObject(Segmentation::SubObject & subobject) {
    const auto & other = std::find_if(std::begin(subobject.objects), std::end(subobject.objects)
    , [&](const uint64_t elemId){
        const auto & elem = objects[elemId];
        return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobject.id;
    });
    if (other == std::end(subobject.objects)) {
        emit beforeAppendRow();
        objects.emplace_back(false, subobject);
        emit appendedRow();
        auto & newObject = objects.back();
        selectObject(newObject);
    } else {
        selectObject(*other);
    }
}

void Segmentation::selectObject(const uint64_t & objectId) {
    if (objectId < objects.size()) {
        selectObject(objects[objectId]);
    }
}

std::size_t Segmentation::selectedObjectsCount() const {
    return selectedObjects.size();
}

void Segmentation::mergelistSave(QIODevice & file) const {
    QTextStream stream(&file);
    for (const auto & obj : objects) {
        stream << obj.id << ' ' << obj.immutable;
        for (const auto & subObj : obj.subobjects) {
            stream << ' ' << subObj.get().id;
        }
        stream << '\n';
        stream << obj.category << '\n';
        stream << obj.comment << '\n';
    }
}

void Segmentation::mergelistLoad(QIODevice & file) {
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);
    QString line;
    blockSignals(true);
    while (!(line = stream.readLine()).isNull()) {
        std::istringstream lineStream(line.toStdString());
        uint64_t objID;
        bool immutable;
        uint64_t initialVolume;
        QString category;
        QString comment;

        bool valid0 = (lineStream >> objID) && (lineStream >> immutable) && (lineStream >> initialVolume);
        bool valid1 = !(category = stream.readLine()).isNull();
        bool valid2 = !(comment = stream.readLine()).isNull();

        if (valid0 && valid1 && valid2) {
            auto & obj = createObject(initialVolume, immutable);
            while (lineStream >> objID) {
                newSubObject(obj, objID);
            }
            std::sort(std::begin(obj.subobjects), std::end(obj.subobjects));
            obj.category = category;
            obj.comment = comment;
        } else {
            qDebug() << "loadMergelist fail";
        }
    }
    blockSignals(false);
    emit resetData();
}

void Segmentation::deleteSelectedObjects() {
    while (!selectedObjects.empty()) {
        removeObject(objects[*std::begin(selectedObjects)]);
    }
}

void Segmentation::mergeSelectedObjects() {
    while (selectedObjects.size() > 1) {
        auto beginIt = std::begin(selectedObjects);
        auto & firstObj = objects[*beginIt];
        auto & secondObj = objects[*std::next(beginIt)];
        //objects are no longer selected when they got merged
        auto flat_deselect = [this](Object & object){
            object.selected = false;
            selectedObjects.erase(object.id);
            emit changedRow(object.id);//deselect
        };
        //(im)mutability possibilities
        if (secondObj.immutable && firstObj.immutable) {
            flat_deselect(firstObj);
            flat_deselect(secondObj);
            if (firstObj.subobjects.size() < secondObj.subobjects.size()) {
                auto & newObj = const_merge(firstObj, secondObj);
                selectedObjects.emplace(newObj.id);
            } else {
                auto & newObj = const_merge(secondObj, firstObj);
                selectedObjects.emplace(newObj.id);
            }
        } else if (secondObj.immutable) {
            flat_deselect(secondObj);
            firstObj.merge(secondObj);
            emit changedRow(firstObj.id);
        } else if (firstObj.immutable) {
            flat_deselect(firstObj);
            secondObj.merge(firstObj);
            emit changedRow(secondObj.id);
        } else {
            if (firstObj.subobjects.size() > secondObj.subobjects.size()) {
                flat_deselect(secondObj);
                firstObj.merge(secondObj);
                removeObject(secondObj);
                emit changedRow(firstObj.id);
            } else {
                flat_deselect(firstObj);
                secondObj.merge(firstObj);
                removeObject(firstObj);
                emit changedRow(secondObj.id);
            }
        }
    }
}

void Segmentation::unmergeSelectedObjects(Segmentation::SubObject & subobjectToUnmerge) {
    const auto & otherIt = std::find_if(std::begin(subobjectToUnmerge.objects), std::end(subobjectToUnmerge.objects)
    , [&](const uint64_t & elemId){
        const auto & elem = objects[elemId];
        return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobjectToUnmerge.id;
    });
    //create object to unmerge if there’s none except the currently selected one
    if (otherIt == std::end(subobjectToUnmerge.objects)) {
        beforeAppendRow();
        objects.emplace_back(false, subobjectToUnmerge);
        appendedRow();
        auto & other = objects.back();
        unmergeSelectedObjects(other);
    } else {
        unmergeSelectedObjects(objects[*otherIt]);
    }
}

void Segmentation::unmergeSelectedObjects(Segmentation::Object & objectToUnmerge) {
    unmergeObject(objects[*std::begin(selectedObjects)], objectToUnmerge);
}
