#include "segmentation.h"

#include "knossos.h"
#include "loader.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QTextStream>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

uint64_t Segmentation::SubObject::highestId = 0;
uint64_t Segmentation::Object::highestId = 0;
uint64_t Segmentation::Object::highestIndex = -1;

Segmentation::Object::Object(Segmentation::SubObject & initialVolume)
    : id(++highestId), todo(false), immutable(false), location(Coordinate(0, 0, 0))
{
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const uint64_t & id, const bool & todo, const bool & immutable, const Coordinate & location, Segmentation::SubObject & initialVolume)
    : id(id), todo(todo), immutable(immutable), location(location)
{
    highestId = std::max(id, highestId);
    addExistingSubObject(initialVolume);
}

Segmentation::Object::Object(const bool & todo, const bool & immutable, const Coordinate & location, std::vector<std::reference_wrapper<SubObject>> initialVolumes)
    : id(++highestId), todo(todo), immutable(immutable), location(location)
{
    for (auto & elem : initialVolumes) {
        addExistingSubObject(elem);
    }
    std::sort(std::begin(subobjects), std::end(subobjects));
}

Segmentation::Object::Object(Object & first, Object & second)
    : id(++highestId), todo(false), immutable(false), location(second.location), selected{true} //merge is selected
{
    subobjects.reserve(first.subobjects.size() + second.subobjects.size());
    merge(first);
    merge(second);
    subobjects.shrink_to_fit();
}

bool Segmentation::Object::operator==(const Segmentation::Object & other) const {
    return index == other.index;
}

void Segmentation::Object::addExistingSubObject(Segmentation::SubObject & sub) {
    subobjects.emplace_back(sub);//add child
    auto objectPosIt = std::lower_bound(std::begin(sub.objects), std::end(sub.objects), this->index);
    sub.objects.emplace(objectPosIt, this->index);//register parent
}

Segmentation::Object & Segmentation::Object::merge(Segmentation::Object & other) {
    for (auto & elem : other.subobjects) {//add parent
        auto & objects = elem.get().objects;
        elem.get().selectedObjectsCount = 1;
        //don’t insert twice
        auto posIt = std::lower_bound(std::begin(objects), std::end(objects), this->index);
        if (posIt == std::end(objects) || *posIt != this->index) {
            objects.emplace(posIt, this->index);
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

Segmentation::Segmentation() : renderAllObjs(true), hoverVersion(false), mouseFocusedObjectId(0) {
    loadOverlayLutFromFile();
}

bool Segmentation::hasSegData() const {
    return hasObjects() || (Loader::Controller::singleton().worker != nullptr && !Loader::Controller::singleton().worker->snappyCache.empty());//we will change smth
}

void Segmentation::clear() {
    state->skeletonState->unsavedChanges |= hasSegData();

    selectedObjectIndices.clear();
    objects.clear();
    Object::highestId = 0;
    Object::highestIndex = -1;
    SubObject::highestId = 0;
    subobjects.clear();
    touched_subobject_id = 0;
    categories = prefixed_categories;

    if (Loader::Controller::singleton().worker != nullptr) {
        //dispatch to loader thread, original cubes are reloaded automatically
        QTimer::singleShot(0, Loader::Controller::singleton().worker.get(), &Loader::Worker::snappyCacheClear);
    }

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

Segmentation::Object & Segmentation::createObject(const uint64_t initialSubobjectId, const Coordinate & location) {
    return createObject(initialSubobjectId, location, ++Object::highestId);
}

Segmentation::Object & Segmentation::createObject(const uint64_t initialSubobjectId, const Coordinate & location, const uint64_t & id, const bool & todo, const bool & immutable) {
    //first is iterator to the newly inserted key-value pair or the already existing value
    auto subobjectIt = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(initialSubobjectId), std::forward_as_tuple(initialSubobjectId)).first;
    auto & subobject = subobjectIt->second;
    emit beforeAppendRow();
    objects.emplace_back(id, todo, immutable, location, subobject); //create object from supervoxel
    emit appendedRow();
    return objects.back();
}

void Segmentation::removeObject(Object & object) {
    unselectObject(object);
    for (auto & elem : object.subobjects) {
        auto & subobject = elem.get();
        subobject.objects.erase(std::remove(std::begin(subobject.objects), std::end(subobject.objects), object.index), std::end(subobject.objects));
        if (subobject.objects.empty()) {
            subobjects.erase(subobject.id);
        }
    }
    //swap with last, so no intermediate rows need to be deleted
    if (objects.size() > 1 && object.index != objects.back().index) {
        //replace object index in subobjects
        for (auto & elem : objects.back().subobjects) {
            auto & subobject = elem.get();
            std::replace(std::begin(subobject.objects), std::end(subobject.objects), objects.back().index, object.index);
        }
        //replace object index in selected objects
        selectedObjectIndices.replace(objects.back().index, object.index);
        std::swap(objects.back().index, object.index);
        std::swap(objects.back(), object);
        emit changedRow(object.index);//object now references the former end
    }
    emit beforeRemoveRow();
    if (object.id == Object::highestId) {
        --Object::highestId;
    }
    objects.pop_back();
    emit removedRow();
    --Object::highestIndex;
}

void Segmentation::changeCategory(Object & obj, const QString & category) {
    obj.category = category;
    emit changedRow(obj.index);
    categories.insert(category);
    emit categoriesChanged();
}

void Segmentation::changeComment(Object & obj, const QString & comment) {
    obj.comment = comment;
    emit changedRow(obj.index);
}

void Segmentation::newSubObject(Object & obj, uint64_t subObjID) {
    auto subobjectIt = subobjects.emplace(std::piecewise_construct, std::forward_as_tuple(subObjID), std::forward_as_tuple(subObjID)).first;
    obj.addExistingSubObject(subobjectIt->second);
}

void Segmentation::setRenderAllObjs(bool all) {
    renderAllObjs = all;
    emit renderAllObjsChanged(renderAllObjs);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::colorOfSelectedObject(const SubObject & subobject) const {
    if (subobject.selectedObjectsCount > 1) {
        return std::make_tuple(255, 0, 0, alpha);//mark overlapping objects in red
    }
    const auto objectIndex = *std::find_if(std::begin(subobject.objects), std::end(subobject.objects), [this](const uint64_t index){
        return objects[index].selected;
    });
    const auto & object = objects[objectIndex];
    const uint8_t red   = overlayColorMap[0][object.index % 256];
    const uint8_t green = overlayColorMap[1][object.index % 256];
    const uint8_t blue  = overlayColorMap[2][object.index % 256];
    return std::make_tuple(red, green, blue, alpha);
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> Segmentation::colorObjectFromId(const uint64_t subObjectID) const {
    if (subObjectID == 0) {
        return std::make_tuple(0, 0, 0, 0);
    }
    const auto subobjectIt = subobjects.find(subObjectID);
    if (subobjectIt == std::end(subobjects)) {
        if (renderAllObjs || selectedObjectIndices.empty()) {
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

    const auto objectIndex = largestObjectContainingSubobject(subobject);
    const uint8_t red   = overlayColorMap[0][objectIndex % 256];
    const uint8_t green = overlayColorMap[1][objectIndex % 256];
    const uint8_t blue  = overlayColorMap[2][objectIndex % 256];
    return std::make_tuple(red, green, blue, alpha);
}

bool Segmentation::subobjectExists(const uint64_t & subobjectId) const {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects);
}

uint64_t Segmentation::subobjectIdOfFirstSelectedObject(const Coordinate & newLocation) {
    if (selectedObjectsCount() != 0) {
        auto & obj = objects[selectedObjectIndices.front()];
        obj.location = newLocation;
        return obj.subobjects.front().get().id;
    } else {
        throw std::runtime_error("no objects selected for subobject retrieval");
    }
}

Segmentation::SubObject & Segmentation::subobjectFromId(const uint64_t & subobjectId, const Coordinate & location) {
    auto it = subobjects.find(subobjectId);//check if subobject exists
    if (it == std::end(subobjects)) {
        createObject(subobjectId, location);//create an object for the selected subobject
        it = subobjects.find(subobjectId);
    }
    return it->second;
}

bool Segmentation::objectOrder(const uint64_t & lhsIndex, const uint64_t & rhsIndex) const {
    const auto & lhs = objects[lhsIndex];
    const auto & rhs = objects[rhsIndex];
    //operator< substitute, prefer immutable objects and choose the smallest
    return (lhs.immutable && !rhs.immutable) || (lhs.immutable == rhs.immutable && lhs.subobjects.size() < rhs.subobjects.size());
}

uint64_t Segmentation::largestObjectContainingSubobjectId(const uint64_t subObjectId, const Coordinate & location) {
    const auto & subobject = subobjectFromId(subObjectId, location);
    return largestObjectContainingSubobject(subobject);
}

uint64_t Segmentation::largestObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    //same comparator for both functions, it seems to work as it is, so i don’t waste my head now to find out why
    //there may have been some reasoning… (at first glance it seems too restrictive for the largest object)
    auto comparator = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectIndex = *std::max_element(std::begin(subobject.objects), std::end(subobject.objects), comparator);
    return objectIndex;
}

uint64_t Segmentation::tryLargestObjectContainingSubobject(const uint64_t subObjectId) const {
    auto it = subobjects.find(subObjectId);
    if(it == std::end(subobjects)) {
        return 0;
    }
    return largestObjectContainingSubobject(it->second);
}

uint64_t Segmentation::smallestImmutableObjectContainingSubobject(const Segmentation::SubObject & subobject) const {
    auto comparitor = std::bind(&Segmentation::objectOrder, this, std::placeholders::_1, std::placeholders::_2);
    const auto objectIndex = *std::min_element(std::begin(subobject.objects), std::end(subobject.objects), comparitor);
    return objectIndex;
}

void Segmentation::hoverSubObject(const uint64_t subobject_id) {
    if (subobject_id != hovered_subobject_id) {
        emit hoveredSubObjectChanged(hovered_subobject_id = subobject_id);
    }
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
        for (const auto & index : it->second.objects) {
            vec.emplace_back(objects[index]);
        }
    }
    return vec;
}

bool Segmentation::isSelected(const SubObject & rhs) const {
    return rhs.selectedObjectsCount != 0;
}

bool Segmentation::isSelected(const uint64_t & objectIndex) const {
    return objects[objectIndex].selected;
}

bool Segmentation::isSubObjectIdSelected(const uint64_t & subobjectId) const {
    auto it = subobjects.find(subobjectId);
    return it != std::end(subobjects) ? isSelected(it->second) : false;
}

void Segmentation::clearObjectSelection() {
    while (!selectedObjectIndices.empty()) {
        unselectObject(selectedObjectIndices.back());
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
    selectedObjectIndices.emplace_back(object.index);
    emit changedRow(object.index);
}

void Segmentation::unselectObject(const uint64_t & objectIndex) {
    if (objectIndex < objects.size()) {
        unselectObject(objects[objectIndex]);
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
    selectedObjectIndices.erase(object.index);
    emit changedRow(object.index);
}

void Segmentation::jumpToObject(Object & object) {
    emit setRecenteringPositionSignal(object.location.x, object.location.y, object.location.z);
    Knossos::sendRemoteSignal();
}

void Segmentation::jumpToObject(const uint64_t & objectIndex) {
    if(objectIndex < objects.size()) {
        jumpToObject(objects[objectIndex]);
    }
}

void Segmentation::selectNextTodoObject() {
    if(selectedObjectIndices.empty() == false) {
        lastTodoObject_id = selectedObjectIndices.front();
        auto & obj = objects[lastTodoObject_id];
        obj.todo = false;
        unselectObject(obj);
    }
    auto list = todolist();
    if(todolist().empty() == false) {
        selectObject(list.front());
        jumpToObject(list.front());
    }
    emit todosLeftChanged();
}

void Segmentation::selectPrevTodoObject() {
    if(selectedObjectIndices.empty() == false) {
        auto & obj = objects[selectedObjectIndices.front()];
        unselectObject(obj);
    }
    auto & obj = objects[lastTodoObject_id];
    obj.todo = true;
    selectObject(obj);
    jumpToObject(obj);
    emit todosLeftChanged();
    return;
}

void Segmentation::markSelectedObjectForSplitting(const Coordinate & pos) {
    if(selectedObjectIndices.empty() == false) {
        auto & object = objects[selectedObjectIndices.front()];
        object.comment = "Split request";
        object.location = pos;
        emit changedRow(selectedObjectIndices.front());
        selectNextTodoObject();
    }
}

std::vector<std::reference_wrapper<Segmentation::Object>> Segmentation::todolist() {
    std::vector<std::reference_wrapper<Segmentation::Object>> todolist;
    for (auto & obj : objects) {
        if(obj.todo) {
            todolist.push_back(obj);
        }
    }
    std::sort(todolist.begin(), todolist.end());
    return todolist;
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
                objects.erase(std::remove(std::begin(objects), std::end(objects), object.index), std::end(objects));//remove parent
            }
            std::swap(object.subobjects, tmp);
            selectObject(object);
            emit changedRow(object.index);
        }
    }
}

Segmentation::Object & Segmentation::objectFromSubobject(Segmentation::SubObject & subobject, const Coordinate & position) {
    const auto & other = std::find_if(std::begin(subobject.objects), std::end(subobject.objects)
    , [&](const uint64_t elemId){
        const auto & elem = objects[elemId];
        return elem.subobjects.size() == 1 && elem.subobjects.front().get().id == subobject.id;
    });
    if (other == std::end(subobject.objects)) {
        emit beforeAppendRow();
        objects.emplace_back(++Object::highestId, false, false, position, subobject);
        emit appendedRow();
        return objects.back();
    } else {
        return objects[*other];
    }
}

void Segmentation::selectObjectFromSubObject(Segmentation::SubObject & subobject, const Coordinate & position) {
    selectObject(objectFromSubobject(subobject, position));
}

void Segmentation::selectObjectFromSubObject(const uint64_t soid, const Coordinate & position) {
    selectObject(objectFromSubobject(subobjectFromId(soid, position), position));
}

void Segmentation::selectObject(const uint64_t & objectIndex) {
    if (objectIndex < objects.size()) {
        selectObject(objects[objectIndex]);
    }
}

std::size_t Segmentation::selectedObjectsCount() const {
    return selectedObjectIndices.size();
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
    if (stream.status() != QTextStream::Ok) {
        qDebug() << "mergelistSave fail";
    }
}

void Segmentation::mergelistLoad(QIODevice & file) {
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);
    QString line;
    blockSignals(true);
    while (!(line = stream.readLine()).isNull()) {
        std::istringstream lineStream(line.toStdString());
        std::istringstream coordLineStream(stream.readLine().toStdString());

        uint64_t objId;
        bool todo;
        bool immutable;
        Coordinate location;
        uint64_t initialVolume;
        QString category;
        QString comment;

        bool valid0 = (lineStream >> objId) && (lineStream >> todo) && (lineStream >> immutable) && (lineStream >> initialVolume);
        bool valid1 = (coordLineStream >> location.x) && (coordLineStream >> location.y) && (coordLineStream >> location.z);
        bool valid2 = !(category = stream.readLine()).isNull();
        bool valid3 = !(comment = stream.readLine()).isNull();

        if (valid0 && valid1 && valid2 && valid3) {
            auto & obj = createObject(initialVolume, location, objId, todo, immutable);
            while (lineStream >> objId) {
                newSubObject(obj, objId);
            }
            std::sort(std::begin(obj.subobjects), std::end(obj.subobjects));
            changeCategory(obj, category);
            obj.comment = comment;
        } else {
            qDebug() << "mergelistLoad fail";
            break;
        }
    }
    blockSignals(false);
    emit resetData();
}

void Segmentation::jobLoad(QIODevice & file) {
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&file);
    QString job_line = stream.readLine();
    QString campaign_line = stream.readLine();
    QString worker_line = stream.readLine();
    QString submit_line = stream.readLine();
    job.id = job_line.isNull()? 0 : job_line.toInt();
    job.campaign = campaign_line.isNull() ? "" : campaign_line;
    job.worker = worker_line.isNull() ? "" : worker_line;
    job.submitPath = submit_line.isNull() ? "" : submit_line;
    job.active = (job.id == 0) ? false : true;
}

void Segmentation::jobSave(QIODevice &file) const {
    QTextStream stream(&file);
    stream << job.id << '\n';
    stream << job.campaign << '\n';
    stream << job.worker << '\n';
    stream << job.submitPath << '\n';
    if (stream.status() != QTextStream::Ok) {
        qDebug() << "mergelistSave fail";
    }
}

void Segmentation::startJobMode() {
    alpha = 37;
    Segmentation::singleton().selectNextTodoObject();
    setRenderAllObjs(false);
}

void Segmentation::deleteSelectedObjects() {
    while (!selectedObjectIndices.empty()) {
        removeObject(objects[selectedObjectIndices.back()]);
    }
}

void Segmentation::mergeSelectedObjects() {
    while (selectedObjectIndices.size() > 1) {
        auto & firstObj = objects[selectedObjectIndices.front()];//front is the merge origin
        auto & secondObj = objects[selectedObjectIndices.back()];
        //objects are no longer selected when they got merged
        auto flat_deselect = [this](Object & object){
            object.selected = false;
            selectedObjectIndices.erase(object.index);
            emit changedRow(object.index);//deselect
        };
        //4 (im)mutability possibilities
        if (secondObj.immutable && firstObj.immutable) {
            flat_deselect(secondObj);

            uint64_t newid;
            newid = const_merge(secondObj, firstObj).index; // merge second object into first object
            secondObj.todo = false;

            selectedObjectIndices.emplace_back(newid);
            //move new index to front, so it gets the new merge origin
            swap(selectedObjectIndices.back(), selectedObjectIndices.front());
            //delay deselection after we swapped new with first
            flat_deselect(firstObj);
        } else if (secondObj.immutable) {
            flat_deselect(secondObj);
            firstObj.merge(secondObj);
            secondObj.todo = false;
            emit changedRow(firstObj.index);
        } else if (firstObj.immutable) {
            flat_deselect(firstObj);
            secondObj.merge(firstObj);
            firstObj.todo = false;
            emit changedRow(secondObj.index);
        } else {//if both are mutable the second object is merged into the first
            flat_deselect(secondObj);
            firstObj.merge(secondObj);
            secondObj.todo = false;
            emit changedRow(firstObj.index);
            removeObject(secondObj);
        }
    }
    emit todosLeftChanged();
}

void Segmentation::unmergeSelectedObjects(const Coordinate & clickPos) {
    if (selectedObjectIndices.size() == 1) {
        deleteSelectedObjects();
    } else while (selectedObjectIndices.size() > 1) {
        auto & objectToUnmerge = objects[selectedObjectIndices.back()];
        unmergeObject(objects[selectedObjectIndices.front()], objectToUnmerge, clickPos);
        unselectObject(objectToUnmerge);
        objectToUnmerge.todo = true;
    }
    emit todosLeftChanged();
}

void Segmentation::jumpToSelectedObject() {
    if (!selectedObjectIndices.empty()) {
        jumpToObject(objects[selectedObjectIndices.front()]);
    }
}

void Segmentation::placeCommentForSelectedObject(const QString & comment) {
    if(selectedObjectIndices.size() == 1) {
        int index = selectedObjectIndices.front();
        objects[index].comment = comment;
        emit changedRow(index);
    }
}
