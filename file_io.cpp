#include "file_io.h"

#include "knossos.h"

#include <ctime>

#include <QStandardPaths>

#include <quazip/quazipfile.h>

#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

QString annotationFileDefaultName() {
    // Generate a default file name based on date and time.
    auto currentTime = time(nullptr);
    auto localTime = localtime(&currentTime);
    return QString("annotation-%1%2%3T%4%5.000.k.zip")
            //value, right aligned padded to width 2, base 10, filled with '0'
            .arg(localTime->tm_year % 100, 2, 10, QLatin1Char('0'))//years from 1900 % 100 = years from 2000
            .arg(localTime->tm_mon + 1, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_mday, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_hour, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_min, 2, 10, QLatin1Char('0'));
}

QString annotationFileDefaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/annotationFiles/" + annotationFileDefaultName();
}

void annotationFileLoad(const QString & filename, const QString & treeCmtOnMultiLoad, bool *isSuccess) {
    bool annotationSuccess = false;
    bool mergelistSuccess = false;
    QRegExp cubeRegEx = QRegExp(R"regex(.*x([0-9]*)y([0-9]*)z([0-9]*)((\.seg\.sz)|(\.segmentation\.snappy)))regex");
    QuaZip archive(filename);
    if (archive.open(QuaZip::mdUnzip)) {
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) {
            QuaZipFile file(&archive);
            const auto & fileInside = archive.getCurrentFileName();
            if (fileInside == "annotation.xml") {
                state->viewer->skeletonizer->loadXmlSkeleton(file, treeCmtOnMultiLoad);
                annotationSuccess = true;
            }
            if (fileInside == "mergelist.txt") {
                Segmentation::singleton().mergelistLoad(file);
                mergelistSuccess = true;
            }
            if (fileInside == "microworker.txt") {
                Segmentation::singleton().jobLoad(file);
            }
            if (cubeRegEx.exactMatch(fileInside)) {
                file.open(QIODevice::ReadOnly);
                auto cube = file.readAll();
                qDebug() << "cube size" << cube.size();
                const auto cubeCoord = CoordOfCube(cubeRegEx.cap(1).toInt(), cubeRegEx.cap(2).toInt(), cubeRegEx.cap(3).toInt());
                Loader::Controller::singleton().worker->snappyCache.emplace(std::piecewise_construct, std::forward_as_tuple(cubeCoord), std::forward_as_tuple(cube));
            }
        }
        state->viewer->changeDatasetMag(DATA_SET);//load from snappy cache
    } else {
        qDebug() << "opening" << filename << "for reading failed";
    }

    if (NULL != isSuccess) {
        *isSuccess = mergelistSuccess || annotationSuccess;
    }
}

void annotationFileSave(const QString & filename, bool *isSuccess) {
    bool allSuccess = true;
    QuaZip archive_write(filename);
    if (archive_write.open(QuaZip::mdCreate)) {
        auto zipCreateFile = [](QuaZipFile & file_write, const QString & name, const int level){
            auto fileinfo = QuaZipNewInfo(name);
            //without permissions set, some archive utilities will not grant any on extract
            fileinfo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
            return file_write.open(QIODevice::WriteOnly, fileinfo, nullptr, 0, Z_DEFLATED, level);
        };
        QuaZipFile file_write(&archive_write);
        const bool open = zipCreateFile(file_write, "annotation.xml", 1);
        if (open) {
            state->viewer->skeletonizer->saveXmlSkeleton(file_write);
        } else {
            qDebug() << filename << "saving nml failed";
            allSuccess = false;
        }
        if (Segmentation::singleton().hasObjects()) {
            QuaZipFile file_write(&archive_write);
            const bool open = zipCreateFile(file_write, "mergelist.txt", 1);
            if (open) {
                Segmentation::singleton().mergelistSave(file_write);
            } else {
                qDebug() << filename << "saving mergelist failed";
                allSuccess = false;
            }
        }
        if (Segmentation::singleton().job.active) {
            QuaZipFile file_write(&archive_write);
            const bool open = zipCreateFile(file_write, "microworker.txt", 1);
            if (open) {
                Segmentation::singleton().jobSave(file_write);
            } else {
                qDebug() << filename << "saving segmentation job failed";
                allSuccess = false;
            }
        }
        QTime time;
        time.start();
        Loader::Controller::singleton().worker->snappyCacheFlush();
        qDebug() << "flush" << time.restart();
        for (const auto & pair : Loader::Controller::singleton().worker->snappyCache) {
            QuaZipFile file_write(&archive_write);
            const auto cubeCoord = pair.first;
            const auto name = QString("%1_mag%2x%3y%4z%5.seg.sz").arg(state->name).arg(1).arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z);
            const bool open = zipCreateFile(file_write, name, 1);
            if (open) {
                file_write.write(pair.second.c_str(), pair.second.length());
            } else {
                qDebug() << filename << "saving snappy cube failed";
                allSuccess = false;
            }
        }
        qDebug() << "save" << time.restart();
    } else {
        qDebug() << "opening" << filename << " for writing failed";
        allSuccess = false;
    }

    if (allSuccess) {
        state->skeletonState->unsavedChanges = false;
    }

    if (NULL != isSuccess) {
        *isSuccess = allSuccess;
    }
}

void nmlExport(const QString & filename) {
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    state->viewer->skeletonizer->saveXmlSkeleton(file);
    file.close();
}

/** This is a replacement for the old updateFileName
    It decides if a skeleton file has a revision(case 1) or not(case2).
    if case1 the revision substring is extracted, incremented and will be replaced.
    if case2 an initial revision will be inserted.
    This method is actually only needed for the save or save as slots, if incrementFileName is selected
*/
void updateFileName(QString & fileName) {
    QRegExp versionRegEx = QRegExp("(\\.)([0-9]{3})(\\.)");
    if (versionRegEx.indexIn(fileName) != -1) {
        const auto versionIndex = versionRegEx.pos(2);//get second regex aka version without file extension
        const auto incrementedVersion = fileName.midRef(versionIndex, 3).toInt() + 1;//3 chars following the dot
        fileName.replace(versionIndex, 3, QString("%1").arg(incrementedVersion, 3, 10, QChar('0')));//pad with zeroes
    } else {
        QFileInfo info(fileName);
        fileName = info.dir().absolutePath() + "/" + info.baseName() + ".001." + info.completeSuffix();
    }
}
