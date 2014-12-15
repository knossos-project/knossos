#include "segmentation.h"

#include "knossos.h"
#include "loader.h"
#include "viewer.h"

#include <QTextStream>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

uint64_t Segmentation::SubObject::highestId = 0;
uint64_t Segmentation::Object::highestId = -1;

Segmentation::Object::Object(Segmentation::SubObject & initialVolume)
    : todo(false), immutable(false), location(Coordinate(0, 0, 0))
{
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const bool & todo, const bool & immutable, const Coordinate & location, Segmentation::SubObject & initialVolume)
    : todo(todo), immutable(immutable), location(location)
{
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const bool & todo, const bool & immutable, const Coordinate & location, std::vector<std::reference_wrapper<SubObject>> initialVolumes)
    : todo(todo), immutable(immutable), location(location)
{
    for (auto & elem : initialVolumes) {
        addExistingSubObject(elem);
    }
    std::sort(std::begin(subobjects), std::end(subobjects));
}

Segmentation::Object::Object(Object & first, Object & second)
    : immutable(false), selected{true}, location(second.location) //merge is selected
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
    tmp.reserve(subobjects.size() + other.subobjects.size());
    std::merge(std::begin(subobjects), std::end(subobjects), std::begin(other.subobjects), std::end(other.subobjects), std::back_inserter(tmp));
    tmp.shrink_to_fit();
    std::swap(subobjects, tmp);
    subobjects.erase(std::unique(std::begin(subobjects), std::end(subobjects)), std::end(subobjects));
    return *this;
}

Segmentation & Segmentation::singleton() {
    static Segmentation segmentation;
    return segmentation;
}

Segmentation::Segmentation() : renderAllObjs(true), segmentationMode(true) {
    loadOverlayLutFromFile();
}

void Segmentation::clear() {
    selectedObjectIds.clear();
    objects.clear();
    Object::highestId = -1;
    SubObject::highestId = 0;
    subobjects.clear();
    touched_subobject_id = 0;

    loader->snappyCacheClear();
    state->viewer->changeDatasetMag(DATA_SET);//reload segmentation cubes

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
            std::move(std::begin(buffer), std::end(buffer), &overlayColorMap[0][0]);
            qDebug() << "sucessfully loaded »stdOverlay.lut«";
        } else {
            qDebug() << "stdOverlay.lut corrupted: expected" << expectedSize << "bytes got" << buffer.size() << "bytes";
        }
    } else {
        qDebug() << "could not open »stdOverlay.lut«";
    }
}

bool Segmentation::hasObjects() const {
    return !this->objects.empty();
}

void Segmentation::createAndSelectObject(const Coordinate & position) {
    clearObjectSelection();
    auto & newObject = createObject(SubObject::highestId+1, position);
    selectObject(newObject);
}

Segmentation::Object & Segmentation::createObject(const uint64_t initialSubobjectId, const Coordinate & location, const bool & todo, const bool & immutable) {
    //first is iterator to the newly inserted key-value pair or the already existing value
    auto subobjectIt = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(initialSubobjectId), std::forward_as_tuple(initialSubobjectId)).first;
    auto & subobject = subobjectIt->second;
    emit beforeAppendRow();
    objects.emplace_back(todo, immutable, location, subobject); //create object from supervoxel
    emit appendedRow();
    return objects.back();
}

void Segmentation::removeObject(Object & object) {
    unselectObject(object);
    for (auto & elem : object.subobjects) {
        auto & subobject = elem.get();
        subobject.objects.erase(std::remove(std::begin(subobject.objects), std::end(subobject.objects), object.id), std::end(subobject.objects));
        if (subobject.objects.empty()) {
            subobjects.erase(subobject.id);
        }
    }
    //swap with last, so no intermediate rows need to be deleted
    if (objects.size() > 1 && object.id != objects.back().id) {
        //replace object id in subobjects
        for (auto & elem : objects.back().subobjects) {
            auto & subobject = elem.get();
            std::replace(std::begin(subobject.objects), std::end(subobject.objects), objects.back().id, object.id);
        }
        //replace object id in selected objects
        selectedObjectIds.replace(objects.back().id, object.id);
        std::swap(objects.back().id, object.id);
        std::swap(objects.back(), object);
        emit changedRow(object.id);//object now references the former end
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
        if (renderAllObjs || selectedObjectIds.empty()) {
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

    const auto objectId = largestObjectContainingSubobject(subobject);
    const uint8_t red   = overlayColorMap[0][objectId % 256];
    const uint8_t green = overlayColorMap[1][objectId % 256];
    const uint8_t blue  = overlayColorMap[2][objectId % 256];
    return std::make_tuple(red, green, blue, alpha);
}

bool Segmentation::subobjectExists(const uint64_t & subobjectId) const {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects);
}

uint64_t Segmentation::subobjectIdOfFirstSelectedObject() {
    return objects[selectedObjectIds.front()].subobjects.front().get().id;
}

Segmentation::SubObject & Segmentation::subobjectFromId(const uint64_t & subobjectId, const Coordinate & location) {
    auto it = subobjects.find(subobjectId);//check if subobject exists
    if (it == std::end(subobjects)) {
        createObject(subobjectId, location);//create an object for the selected subobject
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

uint64_t Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    //same comparitor for both functions, it seems to work as it is, so i don’t waste my head now to find out why
    //there may have been some reasoning… (at first glance it seems too restrictive for the largest object)
    auto comparitor = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectId = *std::max_element(std::begin(subobject.objects), std::end(subobject.objects), comparitor);
    return objectId;
}

uint64_t Segmentation::smallestImmutableObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    auto comparitor = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectId = *std::min_element(std::begin(subobject.objects), std::end(subobject.objects), comparitor);
    return objectId;
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
    std::vector<std::reference_wrapper<Segmentation::Object>> vec;
    if (it != std::end(subobjects)) {
        for (const auto & id : it->second.objects) {
            vec.emplace_back(objects[id]);
        }
    }
    return vec;
}

bool Segmentation::isSelected(const SubObject & rhs) const {
    return rhs.selectedObjectsCount != 0;
}

bool Segmentation::isSelected(const uint64_t & objectId) const {
    return objects[objectId].selected;
}

bool Segmentation::isSubObjectIdSelected(const uint64_t & subobjectId) const {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects) ? isSelected(it->second) : false;
}

void Segmentation::clearObjectSelection() {
    while (!selectedObjectIds.empty()) {
        unselectObject(selectedObjectIds.back());
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
    selectedObjectIds.emplace_back(object.id);
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
    selectedObjectIds.erase(object.id);
    emit changedRow(object.id);
}

void Segmentation::unmergeObject(Segmentation::Object & object, Segmentation::Object & other, const Coordinate & position) {
    decltype(object.subobjects) tmp;
    std::set_difference(std::begin(object.subobjects), std::end(object.subobjects), std::begin(other.subobjects), std::end(other.subobjects), std::back_inserter(tmp));
    if (!tmp.empty()) {//only unmerge if subobjects remain
        if (object.immutable) {
            unselectObject(object);
            emit beforeAppendRow();
            objects.emplace_back(false, false, position, tmp);
            emit appendedRow();
            selectObject(objects.back());
        } else {
            unselectObject(object);
            for (auto & elem : other.subobjects) {
                auto & objects = elem.get().objects;
                objects.erase(std::remove(std::begin(objects), std::end(objects), object.id), std::end(objects));//remove parent
            }
            std::swap(object.subobjects, tmp);
            selectObject(object);
            emit changedRow(object.id);
        }
    }
}

void Segmentation::selectObjectFromSubObject(Segmentation::SubObject & subobject, const Coordinate & position) {
    const auto & other = std::find_if(std::begin(subobject.objects), std::end(subobject.objects)
    , [&](const uint64_t elemId){
        const auto & elem = objects[elemId];
        return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobject.id;
    });
    if (other == std::end(subobject.objects)) {
        emit beforeAppendRow();
        objects.emplace_back(false, false, position, subobject);
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

void Segmentation::jumpToObject(const uint64_t & objectId) {
    if(objectId < objects.size()) {
        state->viewer->userMove(objects[objectId].location.x - state->viewerState->currentPosition.x,
                            objects[objectId].location.y - state->viewerState->currentPosition.y,
                            objects[objectId].location.z - state->viewerState->currentPosition.z,
                            USERMOVE_NEUTRAL, VIEWPORT_UNDEFINED);
    }
}

void Segmentation::updateLocationForFirstSelectedObject(const Coordinate & newLocation) {
    objects[selectedObjectIds.front()].location = newLocation;
}

std::size_t Segmentation::selectedObjectsCount() const {
    return selectedObjectIds.size();
}

void Segmentation::mergelistSave(QIODevice & file) const {
    QTextStream stream(&file);
    for (const auto & obj : objects) {
        stream << obj.id << ' ' << obj.todo << ' ' << obj.immutable;
        for (const auto & subObj : obj.subobjects) {
            stream << ' ' << subObj.get().id;
        }
        stream << '\n';
        stream << obj.location.x << ' ' << obj.location.y << ' ' << obj.location.z << '\n';
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
        bool todo;
        bool immutable;
        Coordinate location;
        uint64_t initialVolume;
        QString category;
        QString comment;

        bool valid0 = (lineStream >> objID) && (lineStream >> todo) && (lineStream >> immutable) && (lineStream >> initialVolume);
        auto coordLine = stream.readLine();
        bool valid1 = false;
        if(coordLine.isNull() == false) {
            std::istringstream coordLineStream(coordLine.toStdString());
            valid1 = (coordLineStream >> location.x) && (coordLineStream >> location.y) && (coordLineStream >> location.z);
        }
        bool valid2 = !(category = stream.readLine()).isNull();
        bool valid3 = !(comment = stream.readLine()).isNull();

        if (valid0 && valid1 && valid2 && valid3) {
            auto & obj = createObject(initialVolume, location, todo, immutable);
            while (lineStream >> objID) {
                newSubObject(obj, objID);
            }
            std::sort(std::begin(obj.subobjects), std::end(obj.subobjects));
            obj.category = category;
            obj.comment = comment;
        } else {
            qDebug() << "loadMergelist fail";
            break;
        }
    }
    blockSignals(false);
    emit resetData();
}

void Segmentation::deleteSelectedObjects() {
    while (!selectedObjectIds.empty()) {
        removeObject(objects[selectedObjectIds.back()]);
    }
}

void Segmentation::mergeSelectedObjects() {
    while (selectedObjectIds.size() > 1) {
        auto & firstObj = objects[selectedObjectIds.front()];//front is the merge origin
        auto & secondObj = objects[selectedObjectIds.back()];
        //objects are no longer selected when they got merged
        auto flat_deselect = [this](Object & object){
            object.selected = false;
            selectedObjectIds.erase(object.id);
            emit changedRow(object.id);//deselect
        };
        //4 (im)mutability possibilities
        if (secondObj.immutable && firstObj.immutable) {
            flat_deselect(secondObj);

            uint64_t newid;
            if (firstObj.subobjects.size() < secondObj.subobjects.size()) {
                newid = const_merge(firstObj, secondObj).id;
            } else {
                newid = const_merge(secondObj, firstObj).id;
            }
            selectedObjectIds.emplace_back(newid);
            //move new id to front, so it gets the new merge origin
            swap(selectedObjectIds.back(), selectedObjectIds.front());
            //delay deselection after we swapped new with first
            flat_deselect(firstObj);
        } else if (secondObj.immutable) {
            flat_deselect(secondObj);
            firstObj.merge(secondObj);
            emit changedRow(firstObj.id);
        } else if (firstObj.immutable) {
            flat_deselect(firstObj);
            secondObj.merge(firstObj);
            emit changedRow(secondObj.id);
        } else {//if both are mutable we can choose the merge order
            if (firstObj.subobjects.size() > secondObj.subobjects.size()) {
                flat_deselect(secondObj);
                firstObj.merge(secondObj);
                emit changedRow(firstObj.id);
                removeObject(secondObj);
            } else {
                flat_deselect(firstObj);
                secondObj.merge(firstObj);
                emit changedRow(secondObj.id);
                removeObject(firstObj);
            }
        }
    }
}

void Segmentation::unmergeSelectedObjects(const Coordinate & clickPos) {
    while (selectedObjectIds.size() > 1) {
        auto & objectToUnmerge = objects[selectedObjectIds.back()];
        unmergeObject(objects[selectedObjectIds.front()], objectToUnmerge, clickPos);
        unselectObject(objectToUnmerge);
    }
}
