/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "file_io.h"

#include "annotation/annotation.h"
#include "loader.h"
#include "widgets/mainwindow.h"
#include "scriptengine/scripting.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"

#include <quazip5/quazipfile.h>

#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QException>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtConcurrentMap>
#include <QTemporaryFile>


#include <ctime>

QString annotationFileDefaultName() {// Generate a default file name based on date and time.
    // ISO 8601 combined date and time in basic format (extended format cannot be used because Windows doesn’t allow ›:‹)
    return QDateTime::currentDateTime().toString("'annotation-'yyyyMMddTHHmm'.000.k.zip'");
}

QString annotationFileDefaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/annotationFiles/" + annotationFileDefaultName();
}

void annotationFileLoad(const QString & filename, bool mergeSkeleton, const QString & treeCmtOnMultiLoad) {
    QElapsedTimer time;
    time.start();
    QSet<QString> nonExtraFiles;
    QRegularExpression cubeRegEx(R"regex(.*mag(?P<mag>[0-9]+)x(?P<x>[0-9]+)y(?P<y>[0-9]+)z(?P<z>[0-9]+)(\.seg\.sz|\.segmentation\.snappy))regex");
    QFile f(filename);
    f.open(QIODevice::ReadOnly);
    auto * fmap = f.map(0, f.size());
    auto data = QByteArray::fromRawData(reinterpret_cast<const char *>(fmap), f.size());
    QBuffer buffer = fmap ? &data : QBuffer{};
    QuaZip archive = fmap ? QuaZip(&buffer) : QuaZip(filename);
    if (!fmap) {
        qDebug() << "loading file without memory map (because it failed)";
    }
    if (archive.open(QuaZip::mdUnzip)) {
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) {
            const auto match = cubeRegEx.match(archive.getCurrentFileName());
            if (match.hasMatch()) {
                if (!Segmentation::singleton().enabled) {
                    state->viewer->window->widgetContainer.datasetLoadWidget.loadDataset(true);// enable overlay
                }
                nonExtraFiles.insert(archive.getCurrentFileName());
                QuaZipFile file(&archive);
                file.open(QIODevice::ReadOnly);
                const auto cubeCoord = CoordOfCube(match.captured("x").toInt(), match.captured("y").toInt(), match.captured("z").toInt());
                const auto anisoMags = Dataset::current().api == Dataset::API::PyKnossos;
                const auto cubeMagnification = anisoMags ? match.captured("mag").toInt() - 1 : static_cast<int>(std::log2(match.captured("mag").toInt()));
                Loader::Controller::singleton().snappyCacheSupplySnappy(cubeCoord, cubeMagnification, file.readAll().toStdString());
            }
        }
        const auto getSpecificFile = [&archive, &nonExtraFiles](const QString & filename, auto func){
            if (archive.setCurrentFile(filename)) {
                nonExtraFiles.insert(archive.getCurrentFileName());
                QuaZipFile file(&archive);
                func(file);
            }
        };
        getSpecificFile("settings.ini", [&archive](auto & file){
            QTemporaryFile tempFile;
            if (file.open(QIODevice::ReadOnly | QIODevice::Text) && tempFile.open()) {
                tempFile.write(Annotation::singleton().extraFiles[archive.getCurrentFileName()] = file.readAll());
                tempFile.close();// QSettings wants to reopen it
                state->mainWindow->loadCustomPreferences(tempFile.fileName());
            } else {
                throw std::runtime_error("couldn’t store custom settings.ini from annotation file to a temporary file");
            }
        });
        getSpecificFile("mergelist.txt", [](auto & file){
            Segmentation::singleton().mergelistLoad(file);
        });
        getSpecificFile("microworker.txt", [](auto & file){
            Segmentation::singleton().jobLoad(file);
        });
        //load skeleton after mergelist as it may depend on a loaded segmentation
        std::unordered_map<decltype(treeListElement::treeID), std::reference_wrapper<treeListElement>> treeMap;
        getSpecificFile("annotation.xml", [&treeMap, mergeSkeleton, treeCmtOnMultiLoad](auto & file){
            treeMap = Skeletonizer::singleton().loadXmlSkeleton(file, mergeSkeleton, treeCmtOnMultiLoad);
        });
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) { // after annotation.xml, because loading .xml clears skeleton
            const QRegularExpression meshRegEx(R"regex([0-9]*\.ply)regex");
            const QRegularExpression nmlRegEx(R"regex(.*\.nml)regex");
            auto fileName = archive.getCurrentFileName();
            if (meshRegEx.match(fileName).hasMatch()) {
                nonExtraFiles.insert(fileName);
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
                        if (!Skeletonizer::singleton().findTreeByTreeID(treeId.get())) {
                            qDebug() << "Tree not found for this mesh, loading as new tree:" << fileName;
                            treeId = boost::none;
                        }
                    }
                }
                Skeletonizer::singleton().loadMesh(file, treeId, fileName);
            }
            if (nmlRegEx.match(fileName).hasMatch()) {// PyK *.nml inside an *.nmx
                nonExtraFiles.insert(fileName);
                QuaZipFile file(&archive);
                Skeletonizer::singleton().loadXmlSkeleton(file, mergeSkeleton, fileName);
                mergeSkeleton = true;// support loading multiple files
            }
        }
        state->viewer->loader_notify();
        for (auto valid = archive.goToFirstFile(); valid; valid = archive.goToNextFile()) {
            if (!nonExtraFiles.contains(archive.getCurrentFileName())) {
                QuaZipFile file(&archive);
                if (archive.getCurrentFileName().endsWith(".py")) {
                    state->scripting->runFile(file, archive.getCurrentFileName(), true);
                }
                file.open(QIODevice::ReadOnly);
                Annotation::singleton().extraFiles[archive.getCurrentFileName()] = file.readAll();
            }
        }
    } else {
        throw std::runtime_error(QObject::tr("opening %1 for reading failed").arg(filename).toStdString());
    }
    qDebug() << "annotationFileLoad took" << time.nsecsElapsed() / 1e9 << "s";
}

void annotationFileSave(const QString & filename, const bool onlySelectedTrees, const bool saveTime, const bool saveDatasetPath) {
    QElapsedTimer time;
    time.start();
    QuaZip archive_write(filename);
    if (archive_write.open(QuaZip::mdCreate)) {
        struct zip_data {
            std::size_t size;
            uLong crc;
            QByteArray compressed_data;
            void deflate(const QByteArray & data) {
                size = data.size();
                crc = crc32(crc32(0L, Z_NULL, 0), reinterpret_cast<const Bytef *>(data.data()), data.size());
                compressed_data = qCompress(data, Z_BEST_SPEED);
                compressed_data.remove(0, 6);// remove 4 byte Qt header and 2 byte zlib header
                compressed_data.remove(compressed_data.size() - 4, 4);// remove 4 byte zlib trailer
            }
        };
        auto zipCreateFile = [](QuaZipFile & file_write, const QString & name, const int level = Z_BEST_SPEED, const uLong size = 0, const quint32 crc = 0){
            auto fileinfo = QuaZipNewInfo(name);
            //without permissions set, some archive utilities will not grant any on extract
            fileinfo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
            fileinfo.uncompressedSize = size;
            return file_write.open(QIODevice::WriteOnly, fileinfo, nullptr, crc, Z_DEFLATED, level, size != 0);
        };
        for (auto it = std::cbegin(Annotation::singleton().extraFiles); it != std::cend(Annotation::singleton().extraFiles); ++it) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, it.key())) {
                file_write.write(it.value());
            } else {
                throw std::runtime_error((filename + ": saving extra file %1 failed").arg(it.key()).toStdString());
            }
        }
        QuaZipFile file_write(&archive_write);
        if (zipCreateFile(file_write, "annotation.xml")) {
            Skeletonizer::singleton().saveXmlSkeleton(file_write, onlySelectedTrees, saveTime, saveDatasetPath);
        } else {
            throw std::runtime_error((filename + ": saving skeleton failed").toStdString());
        }
        if (Segmentation::singleton().hasObjects() && !onlySelectedTrees) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, "mergelist.txt")) {
                Segmentation::singleton().mergelistSave(file_write);
            } else {
                throw std::runtime_error((filename + ": saving mergelist failed").toStdString());
            }
        }
        if (Segmentation::singleton().job.id != 0) {
            QuaZipFile file_write(&archive_write);
            if (zipCreateFile(file_write, "microworker.txt")) {
                Segmentation::singleton().jobSave(file_write);
            } else {
                throw std::runtime_error((filename + ": saving segmentation job failed").toStdString());
            }
        }
        QElapsedTimer time;
        time.start();
        std::vector<std::reference_wrapper<const treeListElement>> trees;
        std::vector<decltype(Skeletonizer::singleton().getMesh(std::declval<treeListElement>()))> mesh_parts;
        for (const auto & tree : state->skeletonState->trees) {
            if ((!onlySelectedTrees || tree.selected) && tree.mesh != nullptr) {
                trees.emplace_back(tree);
                mesh_parts.emplace_back(Skeletonizer::singleton().getMesh(tree));
            }
        }
        qDebug() << "retrieving meshes" << time.nsecsElapsed() / 1e9;
        time.restart();
        std::vector<std::size_t> ids(mesh_parts.size());
        std::vector<zip_data> compressed_meshes(mesh_parts.size());
        std::iota(std::begin(ids), std::end(ids), 0);
        try {
            QtConcurrent::blockingMap(ids, [&trees, &mesh_parts, &compressed_meshes](const auto id){
                QBuffer buffer;
                buffer.open(QIODevice::WriteOnly);
                const auto & [vertex_components, colors, indices] = mesh_parts[id];
                Skeletonizer::singleton().saveMesh(buffer, trees[id], vertex_components, colors, indices);
                buffer.close();
                compressed_meshes[id].deflate(buffer.data());
            });
        } catch (QException & e) {// any exception gets rethrown as Q(Unhandled)Exception but we only handle std::exception upwards
            throw std::runtime_error("couldn’t generate ply");
        }
        qDebug() << "generating ply" << time.nsecsElapsed() / 1e9;
        time.restart();
        for (const auto & id : ids) {
            QuaZipFile file_write(&archive_write);
            const auto filename = QString::number(trees[id].get().treeID) + ".ply";
            if (zipCreateFile(file_write, filename, Z_BEST_SPEED, compressed_meshes[id].size, compressed_meshes[id].crc)) {
                file_write.write(compressed_meshes[id].compressed_data);
            } else {
                throw std::runtime_error((filename + ": saving mesh failed").toStdString());
            }
        }
        qDebug() << "saving ply" << time.nsecsElapsed() / 1e9;
        if (!onlySelectedTrees) {
            QElapsedTimer cubeTime;
            cubeTime.start();
            const auto & cubes = Loader::Controller::singleton().getAllModifiedCubes();
            for (std::size_t i = 0; i < cubes.size(); ++i) {
                const auto mag = Dataset::current().api == Dataset::API::PyKnossos ? i + 1 : std::pow(2, i);
                const auto nameTemplate = QString("%1_mag%2x%3y%4z%5.seg.sz").arg(Dataset::current().experimentname).arg(QString::number(mag));
                for (const auto & pair : cubes[i]) {
                    QuaZipFile file_write(&archive_write);
                    const auto cubeCoord = pair.first;
                    const auto name = nameTemplate.arg(cubeCoord.x).arg(cubeCoord.y).arg(cubeCoord.z);
                    if (zipCreateFile(file_write, name)) {
                        file_write.write(pair.second.c_str(), pair.second.length());
                    } else {
                        throw std::runtime_error((filename + ": saving snappy cube failed").toStdString());
                    }
                }
            }
            qDebug() << "save cubes" << cubeTime.restart();
        }
    } else {
        throw std::runtime_error(QObject::tr("opening %1 for writing failed").arg(filename).toStdString());
    }

    Annotation::singleton().unsavedChanges = false;
    qDebug() << "save" << time.restart();
}

void nmlExport(const QString & filename) {
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    Skeletonizer::singleton().saveXmlSkeleton(file);
    file.close();
}

/** This is a replacement for the old updateFileName
    It decides if a skeleton file has a revision(case 1) or not(case2).
    if case1 the revision substring is extracted, incremented and will be replaced.
    if case2 an initial revision will be inserted.
    This method is actually only needed for the save or save as slots, if incrementFileName is selected
*/
QString updatedFileName(QString fileName) {
    QRegularExpression versionRegEx{R"regex((\.)(?P<version>[0-9]+)(\.))regex"};
    const auto match = versionRegEx.match(fileName);
    if (match.hasMatch()) {
        const auto incrementedVersion = match.capturedRef("version").toInt() + 1;
        return fileName.replace(match.capturedStart("version"), match.capturedLength("version"), QString("%1").arg(incrementedVersion, 3, 10, QChar('0')));// replace version chars, pad with zeroes to minimum length of 3
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
            const auto jarray = json_conf.array();
            if (!json_conf.isArray() || jarray.size() != 256) {//dataset adjustment currently requires 256 values
                kill();
            }
            table.resize(jarray.size());
            for (int i = 0; i < jarray.size(); ++i) {
                table[i] = std::make_tuple(jarray[i][0].toInt(), jarray[i][1].toInt(), (jarray[i][2].toInt()));
            }
        }
    } else {
        kill();
    }
    return table;
}
