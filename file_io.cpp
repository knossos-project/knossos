/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "file_io.h"

#include "loader.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"

#include <quazipfile.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStandardPaths>

#include <ctime>

QString annotationFileDefaultName() {
    // Generate a default file name based on date and time.
    auto currentTime = time(nullptr);
    auto localTime = localtime(&currentTime);
    return QString("annotation-%1%2%3T%4%5.000.k.zip")
            .arg(1900 + localTime->tm_year)
            //value, right aligned padded to width 2, base 10, filled with '0'
            .arg(localTime->tm_mon + 1, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_mday, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_hour, 2, 10, QLatin1Char('0'))
            .arg(localTime->tm_min, 2, 10, QLatin1Char('0'));
}

QString annotationFileDefaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/annotationFiles/" + annotationFileDefaultName();
}

void annotationFileLoad(const QString & filename, const bool mergeSkeleton, const QString & treeCmtOnMultiLoad) {
    QSet<QString> nonExtraFiles;
    QRegularExpression cubeRegEx(R"regex(.*mag(?P<mag>[0-9]*)x(?P<x>[0-9]*)y(?P<y>[0-9]*)z(?P<z>[0-9]*)((\.seg\.sz)|(\.segmentation\.snappy)))regex");
    QuaZip archive(filename);
    if (archive.open(QuaZip::mdUnzip)) {
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) {
            const auto match = cubeRegEx.match(archive.getCurrentFileName());
            if (match.hasMatch()) {
                if (!Segmentation::enabled) {
                    Segmentation::enabled = true;
                    Loader::Controller::singleton().enableOverlay();
                }
                nonExtraFiles.insert(archive.getCurrentFileName());
                QuaZipFile file(&archive);
                file.open(QIODevice::ReadOnly);
                const auto cubeCoord = CoordOfCube(match.captured("x").toInt(), match.captured("y").toInt(), match.captured("z").toInt());
                Loader::Controller::singleton().snappyCacheSupplySnappy(cubeCoord, match.captured("mag").toInt(), file.readAll().toStdString());
            }
        }
        const auto getSpecificFile = [&archive, &nonExtraFiles](const QString & filename, auto func){
            if (archive.setCurrentFile(filename)) {
                nonExtraFiles.insert(archive.getCurrentFileName());
                QuaZipFile file(&archive);
                func(file);
            }
        };
        getSpecificFile("mergelist.txt", [&archive](auto & file){
            Segmentation::singleton().mergelistLoad(file);
        });
        getSpecificFile("microworker.txt", [&archive](auto & file){
            Segmentation::singleton().jobLoad(file);
        });
        //load skeleton after mergelist as it may depend on a loaded segmentation
        std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> treeMap;
        getSpecificFile("annotation.xml", [&archive, &treeMap, mergeSkeleton, treeCmtOnMultiLoad](auto & file){
            treeMap = state->viewer->skeletonizer->loadXmlSkeleton(file, mergeSkeleton, treeCmtOnMultiLoad);
        });
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) { // after annotation.xml, because loading .xml clears skeleton
            const QRegularExpression meshRegEx(R"regex([0-9]*.ply)regex");
            auto fileName = archive.getCurrentFileName();
            const auto matchMesh = meshRegEx.match(fileName);
            if (matchMesh .hasMatch()) {
                nonExtraFiles.insert(archive.getCurrentFileName());
                QuaZipFile file(&archive);
                auto nameWithoutExtension = fileName;
                nameWithoutExtension.chop(4);
                bool validId = false;
                auto treeId = boost::make_optional<std::uint64_t>(nameWithoutExtension.toULongLong(&validId));
                if (!validId) {
                    qDebug() << "Filename not of the form <tree id>.ply, so loading as new tree:" << fileName;
                    treeId = boost::none;
                } else if (mergeSkeleton) {
                    const auto iter = treeMap.find(treeId.get());
                    if (iter != std::end(treeMap)) {
                        treeId = iter->second.get().treeID;
                    } else {
                        qDebug() << "Tree not found for this mesh, loading as new tree:" << fileName;
                        treeId = boost::none;
                    }
                }
                Skeletonizer::singleton().loadMesh(file, treeId, fileName);
            }
        }
        state->viewer->loader_notify();
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) {
            if (!nonExtraFiles.contains(archive.getCurrentFileName())) {
                QuaZipFile file(&archive);
                file.open(QIODevice::ReadOnly);
                Session::singleton().extraFiles[archive.getCurrentFileName()] = file.readAll();
            }
        }
    } else {
        throw std::runtime_error(QObject::tr("opening %1 for reading failed").arg(filename).toStdString());
    }
}

void annotationFileSave(const QString & filename) {
    QTime time;
    time.start();
    QuaZip archive_write(filename);
    if (archive_write.open(QuaZip::mdCreate)) {
        auto zipCreateFile = [](QuaZipFile & file_write, const QString & name, const int level){
            auto fileinfo = QuaZipNewInfo(name);
            //without permissions set, some archive utilities will not grant any on extract
            fileinfo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
            return file_write.open(QIODevice::WriteOnly, fileinfo, nullptr, 0, Z_DEFLATED, level);
        };
        for (auto it = std::cbegin(Session::singleton().extraFiles); it != std::cend(Session::singleton().extraFiles); ++it) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, it.key(), 1)) {
                file_write.write(it.value());
            } else {
                throw std::runtime_error((filename + ": saving extra file %1 failed").arg(it.key()).toStdString());
            }
        }
        QuaZipFile file_write(&archive_write);
        if (zipCreateFile(file_write, "annotation.xml", 1)) {
            state->viewer->skeletonizer->saveXmlSkeleton(file_write);
        } else {
            throw std::runtime_error((filename + ": saving skeleton failed").toStdString());
        }
        if (Segmentation::singleton().hasObjects()) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, "mergelist.txt", 1)) {
                Segmentation::singleton().mergelistSave(file_write);
            } else {
                throw std::runtime_error((filename + ": saving mergelist failed").toStdString());
            }
        }
        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Mode_MergeSimple)) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, "microworker.txt", 1)) {
                Segmentation::singleton().jobSave(file_write);
            } else {
                throw std::runtime_error((filename + ": saving segmentation job failed").toStdString());
            }
        }
        for (const auto & tree : state->skeletonState->trees) {
            if (tree.mesh != nullptr) {
                QuaZipFile file_write(&archive_write);
                const auto filename = QString::number(tree.treeID) + ".ply";
                if (zipCreateFile(file_write, filename, 1)) {
                    Skeletonizer::singleton().saveMesh(file_write, tree);
                } else {
                    throw std::runtime_error((filename + ": saving mesh failed").toStdString());
                }
            }
        }
        QTime cubeTime;
        cubeTime.start();
        const auto & cubes = Loader::Controller::singleton().getAllModifiedCubes();
        for (std::size_t i = 0; i < cubes.size(); ++i) {
            const auto magName = QString("%1_mag%2x%3y%4z%5.seg.sz").arg(state->name).arg(QString::number(std::pow(2, i)));
            for (const auto & pair : cubes[i]) {
                QuaZipFile file_write(&archive_write);
                const auto cubeCoord = pair.first;
                const auto name = magName.arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z);
                if (zipCreateFile(file_write, name, 1)) {
                    file_write.write(pair.second.c_str(), pair.second.length());
                } else {
                    throw std::runtime_error((filename + ": saving snappy cube failed").toStdString());
                }
            }
        }
        qDebug() << "save cubes" << cubeTime.restart();
    } else {
        throw std::runtime_error(QObject::tr("opening %1 for writing failed").arg(filename).toStdString());
    }

    Session::singleton().unsavedChanges = false;
    qDebug() << "save" << time.restart();
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
QString updatedFileName(QString fileName) {
    QRegExp versionRegEx = QRegExp("(\\.)([0-9]{3})(\\.)");
    if (versionRegEx.indexIn(fileName) != -1) {
        const auto versionIndex = versionRegEx.pos(2);//get second regex aka version without file extension
        const auto incrementedVersion = fileName.midRef(versionIndex, 3).toInt() + 1;//3 chars following the dot
        return fileName.replace(versionIndex, 3, QString("%1").arg(incrementedVersion, 3, 10, QChar('0')));//pad with zeroes
    } else {
        QFileInfo info(fileName);
        return info.dir().absolutePath() + "/" + info.baseName() + ".001." + info.completeSuffix();
    }
}

std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> loadLookupTable(const QString & path) {
    auto kill = [&path](){
        const auto msg = QObject::tr("Failed to open LUT file: »%1«").arg(path);
        qWarning() << msg;
        throw std::runtime_error(msg.toUtf8());
    };

    const std::size_t expectedBinaryLutSize = 256 * 3;//RGB
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> table;
    QFile overlayLutFile(path);
    if (overlayLutFile.open(QIODevice::ReadOnly)) {
        if (overlayLutFile.size() == expectedBinaryLutSize) {//imageJ binary LUT
            const auto buffer = overlayLutFile.readAll();
            table.resize(256);
            for (int i = 0; i < 256; ++i) {
                table[i] = std::make_tuple(static_cast<uint8_t>(buffer[i]), static_cast<uint8_t>(buffer[256 + i]), static_cast<uint8_t>(buffer[512 + i]));
            }
        } else {//json
            QJsonDocument json_conf = QJsonDocument::fromJson(overlayLutFile.readAll());
            auto jarray = json_conf.array();
            if (!json_conf.isArray() || jarray.size() != 256) {//dataset adjustment currently requires 256 values
                kill();
            }
            table.resize(jarray.size());
            for (int i = 0; i < jarray.size(); ++i) {
                table[i] = std::make_tuple(jarray[i].toArray()[0].toInt(), jarray[i].toArray()[1].toInt(), (jarray[i].toArray()[2].toInt()));
            }
        }
    } else {
        kill();
    }
    return table;
}
