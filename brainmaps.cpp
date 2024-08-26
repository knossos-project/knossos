/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2018
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

#include "brainmaps.h"
#include "qjsondocument.h"
#include "qoffscreensurface.h"
#include "qprogressbar.h"
#include "qsurface.h"

#include <jwt-cpp/jwt.h>

static std::string getJwt(const std::string & email, const std::string & privateKey) {
    using namespace std::string_literals;
    return jwt::create()
        .set_payload_claim("scope", jwt::claim{"https://www.googleapis.com/auth/brainmaps"s})
        .set_type("JWS")
        //.set_key_id(sacc["private_key_id"].toString().toStdString()) // doesn’t seem to be required
        .set_issuer(email)
        //.set_subject(sacc["client_email"].toString().toStdString()) // doesn’t seem to be required
        .set_audience("https://accounts.google.com/o/oauth2/token")
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
        .sign(jwt::algorithm::rs256{"", privateKey});
}

static void updateSegToken();

#include "network.h"

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

template<bool block>
static auto googleRequest = [](auto token, QUrl path, QByteArray payload = QByteArray{}){
    QNetworkAccessManager & qnam = Network::singleton().manager;
    QNetworkRequest request(path);
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", (QString("Bearer ") + token).toUtf8());
    }
    if (block) {
        request.setPriority(QNetworkRequest::HighPriority);
    }
    QNetworkReply * reply;
    if (payload != QByteArray{} || path.path().endsWith(":create")) {
        if (token.isEmpty()) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        } else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        }
        reply = qnam.post(request, payload);
    } else {
        reply = qnam.get(request);
    }
    QObject::connect(reply, &QNetworkReply::errorOccurred, [](QNetworkReply::NetworkError error){
        if (error == QNetworkReply::AuthenticationRequiredError || error == QNetworkReply::ContentAccessDenied) {
            updateSegToken();
        }
    });
    if constexpr (block) {
        return blockDownloadExtractData(*reply);
    } else {
        return reply;
    }
};

static auto googleRequestAsync = googleRequest<false>;
static auto googleRequestBlocking = googleRequest<true>;

#include <QJsonDocument>

auto getBrainmapsToken = [](const QJsonDocument & sacc) {
    const auto jwt = getJwt(sacc["client_email"].toString().toStdString(), sacc["private_key"].toString().toStdString());
    const auto payload = "grant_type=" + QByteArray("urn:ietf:params:oauth:grant-type:jwt-bearer").toPercentEncoding() + "&assertion=" + QByteArray::fromStdString(jwt);
    const auto pair = googleRequestBlocking(QString{""}, QUrl{"https://accounts.google.com/o/oauth2/token"}, payload);
    qDebug() << QJsonDocument::fromJson(pair.second);
    return std::pair<bool, QString>{pair.first, QJsonDocument::fromJson(pair.second)["access_token"].toString()};
};

#include "dataset.h"

void updateToken(Dataset & layer) {
    auto pair = getBrainmapsToken(layer.brainmapsSacc);
    if (pair.first) {
        qDebug() << "updateToken" << layer.url;
        layer.token = pair.second;
    } else {
        qDebug() << "getBrainmapsToken failed" << pair.second;
        throw std::runtime_error(QObject::tr("couldn’t fetch brainmaps token: \n%1").arg(pair.second).toStdString());
    }
}

#include "segmentation/segmentation.h"

static void updateSegToken() {
    updateToken(Dataset::datasets[Segmentation::singleton().layerId]);
}

void createChangeStack(const Dataset & layer) {
    if (!layer.useAlternativebrainmapsChangeServer && !layer.brainmapsChangeStack.isEmpty()) {
        auto url = layer.url;
        url.setPath(url.path().replace("volumes", "changes") + "/" + layer.brainmapsChangeStack + ":create");
        qDebug() << "changes.create" << layer.brainmapsChangeStack << googleRequestBlocking(layer.token, url);
    }
}

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

void parseGoogleJson(Dataset & info) {
//    qDebug() << "change_stacks" << googleRequestBlocking(info.token, info.url.toString().replace("volumes", "changes") + "/change_stacks").second.data();
//    qDebug() << "meshes" << googleRequestBlocking(info.token, info.url.toString().replace("volumes", "objects") + "/meshes").second.data();

    const auto config = googleRequestBlocking(info.token, info.url);

    if (!config.first) {
        qWarning() << "couldn’t fetch brainmaps config";
        return;
    }

    info.experimentname = QFileInfo{info.url.path()}.fileName().section(':', 2);
    const auto jmap = QJsonDocument::fromJson(config.second).object();

    const auto boundary_json = jmap["geometry"][0]["volumeSize"];
    info.boundary = {
        boundary_json["x"].toString().toInt(),
        boundary_json["y"].toString().toInt(),
        boundary_json["z"].toString().toInt(),
    };

    for (auto scaleRef : jmap["geometry"].toArray()) {
        const auto & scale_json = scaleRef.toObject()["pixelSize"].toObject();
        info.scales.emplace_back(scale_json["x"].toDouble(1), scale_json["y"].toDouble(1), scale_json["z"].toDouble(1));
    }
    info.type = jmap["geometry"][0]["channelType"] == "UINT64" ? Dataset::CubeType::SEGMENTATION_SZ : Dataset::CubeType::RAW_PNG;
    qDebug() << jmap["geometry"].toArray();

    if (info.type == Dataset::CubeType::RAW_PNG) {
        // xy and xyz downscaling are in the same array
        // remove the xy scaling and remember how much to increase the index to get the true mag2
        info.brainmapsMagSkip = info.scales.size();
        info.scales.erase(std::remove_if(std::next(std::begin(info.scales)), std::end(info.scales), [&info](auto scale){
            return scale.z == info.scales[0].z;
        }), std::end(info.scales));
        info.brainmapsMagSkip -= info.scales.size();
    }
    info.scale = info.scales.front();

    info.magIndex = 0;
    info.lowestAvailableMagIndex = 0;
    info.highestAvailableMagIndex = jmap["geometry"].toArray().size() - 1; //highest google mag
}

bool bminvalid(const bool erase, const std::uint64_t srcSvx, const std::uint64_t dstSvx) {
    const auto mergeUnfit = !erase && (Segmentation::singleton().isSubObjectIdSelected(srcSvx) && Segmentation::singleton().isSubObjectIdSelected(dstSvx));
    const auto splitUnfit = erase && !(Segmentation::singleton().isSubObjectIdSelected(srcSvx) && Segmentation::singleton().isSubObjectIdSelected(dstSvx));
    return srcSvx == dstSvx || srcSvx == Segmentation::singleton().backgroundId || dstSvx == Segmentation::singleton().backgroundId || mergeUnfit || splitUnfit;
}

struct download_data_t {
    struct mesh_data_t {
        std::vector<float> vertices;
        std::vector<std::uint32_t> indices;
        std::size_t index_offset{0};
    };
    std::unordered_map<std::uint64_t, mesh_data_t> mesh_data;
    std::unordered_set<QNetworkReply*> replies;
    std::ptrdiff_t size{0};
};
using download_list = std::unordered_map<std::uint64_t, download_data_t>;

#include <QDataStream>
#include <QNetworkReply>

static auto parseBinaryMesh = [](QNetworkReply & reply, auto & meshdata){
    if (reply.error() != QNetworkReply::NoError) {
        qDebug() << reply.error() << reply.errorString() << reply.readAll().constData();
        return;
    }
    QDataStream ds(reply.readAll());
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);
    while (!ds.atEnd()) {
        quint64 soid, verts, idx;
        ds >> soid;
        auto fragkeys = soid;
        while (soid == fragkeys) {
            ds >> fragkeys;
        }
        for (std::size_t i{0}; i < fragkeys; ++i) {
            quint8 frag;
            ds >> frag;
        }
        ds >> verts >> idx;
        auto & vertices = meshdata[soid].vertices;
        auto & indices = meshdata[soid].indices;
        auto & index_offset = meshdata[soid].index_offset;
        vertices.reserve(vertices.size() + verts);
        indices.reserve(indices.size() + idx);
        for (std::size_t i{0}; i < verts; ++i) {
            float x, y, z;
            ds >> x >> y >> z;
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
        for (std::size_t i{0}; i < idx; ++i) {
            std::uint32_t idx, idy, idz;
            ds >> idx >> idy >> idz;
            indices.push_back(idx + index_offset);
            indices.push_back(idy + index_offset);
            indices.push_back(idz + index_offset);
        }
        index_offset = vertices.size() / 3;
    }
};

#include "skeleton/skeletonizer.h"

#include <QProgressBar>

static void updateAnnotation(download_list & download_data, QElapsedTimer & timer, std::uint64_t soid, QProgressBar & downloadProgress, QProgressBar & addProgress) {
    qDebug() << "mesh" << soid << "fetch" << download_data[soid].size/1e6 << "MB in" << timer.elapsed() << "ms";
    addProgress.setMaximum(download_data[soid].mesh_data.size());
    addProgress.setValue(0);
    addProgress.show();
    for (auto && pair : download_data[soid].mesh_data) {
        addProgress.setValue(addProgress.value() + 1);
        QVector<float> normals;
        QVector<std::uint8_t> colors;
        if (pair.second.vertices.size() < (1<<29) && pair.second.indices.size() < (1<<29)) {
            auto verts = QVector<float>(std::cbegin(pair.second.vertices), std::cend(pair.second.vertices));
            auto idc = QVector<std::uint32_t>(std::cbegin(pair.second.indices), std::cend(pair.second.indices));
            Skeletonizer::singleton().addMeshToTree(pair.first, verts, normals, idc, colors);
        } else {
            qWarning() << pair.first << ": mesh too big";
        }
    }
    qDebug() << "mesh" << soid << "add in" << timer.restart() << "ms";

    download_data.erase(soid);
    if (download_data.empty()) {
        QTimer::singleShot(450, [&download_data, &downloadProgress, &addProgress](){
            downloadProgress.setVisible(!download_data.empty());
            addProgress.hide();
        });
    } else {
        for (auto & pair : download_data) {
            std::cout << ' ' << pair.first;
        }
        std::cout << std::endl;
    }
}

template<bool isDirectConnectivity>
static QNetworkReply * fragmentListRequest(const Dataset & dataset, const std::uint64_t soid) {
    auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "objects")
        + QString("/meshes/%1:listfragments?objectId=%2&returnSupervoxelIds=true")
        .arg(dataset.brainmapsMeshKey).arg(soid);
    if (!dataset.brainmapsChangeStack.isEmpty() && !isDirectConnectivity) {
        url += QString{"&header.changeStackId=%1"}.arg(dataset.brainmapsChangeStack);
    }
    qDebug() << "request fragments for" << soid;
    return dataset.useAlternativebrainmapsChangeServer ? Network::singleton().manager.get(QNetworkRequest{url}) : googleRequestAsync(dataset.token, url);
}

#include <QApplication>
#include <QMessageBox>

void requestRefreshOrErrorMsg(QNetworkReply & reply, const std::uint64_t soid, const QString & failText) {
    if (reply.error() == QNetworkReply::NoError) {
        retrieveMeshes(soid);
    }
    else {
        QMessageBox errorMsg{QApplication::activeWindow()};
        errorMsg.setIcon(QMessageBox::Warning);
        errorMsg.setText(failText);
        errorMsg.setDetailedText(QObject::tr("Server response: %1\n%2").arg(reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()).arg(QString::fromUtf8(reply.readAll())));
        errorMsg.exec();
        qDebug() << failText << reply.readAll();
    }
}

void anchorSetNoteRequest(const std::uint64_t anchorid, const QString & note) {
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString("/anchors:addnote?anchorId=%1").arg(anchorid);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto * reply = Network::singleton().manager.post(request, QString{R"json({"note": "%1"})json"}.arg(note).toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, [reply, anchorid]() {
        if (reply->error() == QNetworkReply::ContentConflictError) {
            const std::uint64_t anchorID = QString::fromUtf8(reply->readAll()).toULongLong();
            QMessageBox errorMsg{QApplication::activeWindow()};
            errorMsg.setIcon(QMessageBox::Warning);
            errorMsg.setText(QObject::tr("Note already exists. Show conflicting anchor mesh?"));
            const QString failText{QObject::tr("Conflicting anchor ID: %1").arg(anchorID)};
            errorMsg.setDetailedText(failText);
            const auto * showButton = errorMsg.addButton(QObject::tr("Yes, show conflicting achor"), QMessageBox::YesRole);
            errorMsg.addButton(QObject::tr("No, close"), QMessageBox::NoRole);
            errorMsg.exec();
            qDebug() << failText << reply->readAll();
            if (errorMsg.clickedButton() == showButton) {
                retrieveMeshes(anchorID);
            }
        } else {
            requestRefreshOrErrorMsg(*reply, anchorid, QObject::tr("Failed to upload new anchor note."));
        }
    });
}

void anchorRequest(const std::uint64_t anchorid, const bool add) {
    const QString operation = add? "add" : "delete";
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString("/anchors:%1").arg(operation);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto * reply = Network::singleton().manager.post(request,  QString{R"json({"anchorId": %1})json"}.arg(anchorid).toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, [reply, anchorid, operation]() {
        requestRefreshOrErrorMsg(*reply, anchorid, QObject::tr("Failed to %1 anchor.").arg(operation));
    });
}

#include "stateInfo.h"
#include <QRegularExpression>
void selectAnchorObjects() {
    QSet<treeListElement *> anchorTrees;
    for (QHashIterator<QString, QVariant> iter(state->skeletonState->selectedTrees.front()->properties); iter.hasNext();) {
        iter.next();
        QRegularExpression exp{"×anchor[1-9]+"};
        if (iter.key() == "anchor" || exp.match(iter.key()).hasMatch()) {
            anchorTrees.insert(Skeletonizer::singleton().findTreeByTreeID(iter.value().toULongLong()));
        }
    }
    Skeletonizer::singleton().select(anchorTrees);
}

void explodeRequest(const std::uint64_t svId) {
    QElapsedTimer timer;
    timer.start();
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString("/%1/explode?svId=%2").arg(dataset.brainmapsChangeStack).arg(svId);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    auto * reply = Network::singleton().manager.post(request, "");
    QObject::connect(reply, &QNetworkReply::finished, [timer, reply, svId]() {
        const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        auto failMsg = QObject::tr("Failed to explode object with Supervoxel %1.").arg(svId);
        if (statusCode == 413) {
            failMsg += "\nObject is too large. Please contact support for this explosion.";
        }
        requestRefreshOrErrorMsg(*reply, svId, failMsg);
        if (reply->error() == QNetworkReply::NoError) {
            const auto jdoc = QJsonDocument::fromJson(reply->readAll());
            auto deletedEdges = jdoc["deleted_edges"].toInt();
            int supervoxels = jdoc["supervoxels"].toInt();
            const auto explosionReport = QObject::tr("Deleted %1 edges from %2 supervoxels in %3 seconds.").arg(deletedEdges).arg(supervoxels).arg(timer.elapsed() / 1000.f, 0, 'g', 2);
            QMessageBox explosionMsg{QApplication::activeWindow()};
            explosionMsg.setIcon(QMessageBox::Information);
            explosionMsg.setText(explosionReport);
            explosionMsg.exec();
            qDebug() << explosionReport;
        }
    });
}

#include <QBuffer>
#include <QJsonArray>
#include <QJsonObject>

#include <boost/range/combine.hpp>

#include "widgets/mainwindow.h" // enabled + progress

static boost::optional<std::uint64_t> brainmapsDownload{boost::none};

#include "viewer.h" // reslice_notify

void resetBusyIfMatch(const std::uint64_t soid) {
    if (brainmapsDownload && brainmapsDownload.get() == soid) {
        brainmapsDownload = boost::none;
        state->viewer->segmentation_changed();
    }
}

bool abortMeshDownload = false;

void selectTrees(std::uint64_t soid, boost::optional<decltype(treeListElement::properties)> properties = boost::none) {
    auto & seg = Segmentation::singleton();

    for (auto & elem : Skeletonizer::singleton().skeletonState.selectedTrees) {
        elem->selected = false;
    }
    Skeletonizer::singleton().skeletonState.selectedTrees.clear();
    if (seg.subobjectExists(soid)) {
        const auto oid = seg.largestObjectContainingSubobjectId(soid, {});
        qDebug() << "selectTrees" << soid << oid;
        QSignalBlocker bs2{Skeletonizer::singleton()};

        for (const auto & so : seg.objects[oid].subobjects) {
            const auto soid = so.get().id;
            auto * tree = Skeletonizer::singleton().findTreeByTreeID(soid);
            if (properties) {
                if (!tree) {
                    tree = &Skeletonizer::singleton().addTree(soid);
                }
                tree->properties = properties.get();
            }
            if (tree) {// filter out existing trees/meshes
                Skeletonizer::singleton().skeletonState.selectedTrees.emplace_back(tree);
                tree->selected = true;
            }
        }
        Segmentation::singleton().selectObject(oid);
    }
    emit Skeletonizer::singleton().resetData();
}

void fetchAgglo(const Dataset & dataset, const std::uint64_t soid, QElapsedTimer & timer, const bool dc) {
    selectTrees(soid);

    QNetworkRequest request{dataset.aggloServer.toString() + QString{"/mergelist/%1?cc=%2"}.arg(soid).arg(dc ? "false" : "true")};
    request.setHeader(QNetworkRequest::UserAgentHeader, "KNOSSOS");
    auto * const reply = Network::singleton().manager.get(request);
    qDebug() << request.url();

    std::vector<quint64> anchors;
    QObject::connect(reply, &QNetworkReply::finished, [reply, soid, &dataset, &downloadProgress = state->mainWindow->meshDownloadProgressBar, timer]() {
        resetBusyIfMatch(soid);
        reply->deleteLater();
        auto & seg = Segmentation::singleton();
        QSignalBlocker bs{seg};
        const auto pos = seg.objects[seg.selectedObjectIndices.front()].location;
        bool anchorLoadedForTheFirstTime{true};
        int recomlod{3};
        std::vector<quint64> anchors;
        decltype(treeListElement::properties) properties;
        if (reply->error() == QNetworkReply::NoError) {
            const auto jdoc = QJsonDocument::fromJson(reply->readAll());

            if (seg.selectedObjectsCount() == 0) {
                qDebug() << "nothing selected";
                return;
            }
            const auto bak = Segmentation::singleton().hovered_subobject_id;// drag
            Segmentation::singleton().hovered_subobject_id = bak;

            auto mergelistStr = jdoc["mergelist"].toString();
            recomlod = jdoc["recommended_lod"].toInt();
            qDebug() << jdoc.object().keys() << jdoc["recommended_lod"].toInt() << timer.nsecsElapsed()/1e9 << "s";
            mergelistStr.insert(mergelistStr.indexOf('\n')+1, QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z));
            while (seg.subobjectExists(soid)) {
                seg.removeObject(seg.objects[seg.subobjectFromId(soid, pos).objects.front()]);
            }
            seg.mergelistLoad(QTextStream{&mergelistStr});

            {
                auto anchorsObj = jdoc["anchor"].toObject();
                auto timestampValue = jdoc["timestamp"];
                auto user = jdoc["user"];
                std::optional<QString> timestamp;
                QString note;
                std::optional<quint64> nuc;
                if (!anchorsObj.empty()) {
                    auto nucValue = anchorsObj.begin().value().toObject().value("nuc");
                    if (!nucValue.isUndefined()) {
                        nuc = nucValue.toVariant().toULongLong();
                    }
                    note = anchorsObj.begin().value().toObject().value("note").toString();
                    for (auto iter = anchorsObj.begin(); iter != anchorsObj.end(); iter++) {
                        anchors.push_back(iter.key().toULongLong());
                    }
                }
                if (!user.isUndefined() && !user.isNull()) {
                    timestamp = QString("%1 %2").arg(QDateTime::fromSecsSinceEpoch(timestampValue.toVariant().toULongLong()).toString("yyyy-MM-ddThh:mm"), user.toString());
                }

                if (!anchors.empty()) {
                    properties["anchor"] = anchors[0];
                    if (nuc.has_value()) {
                        properties["nuc"] = nuc.value();
                    }
                    if (!note.isEmpty()) {
                        properties["note"] = note;
                    }
                }
                if (timestamp.has_value()) {
                    properties["t"] = timestamp.value();
                }
                if (anchors.size() > 1) {
                    for (std::size_t i = 1; i < anchors.size(); i++) {
                        anchorLoadedForTheFirstTime = Skeletonizer::singleton().findTreeByTreeID(anchors[i]) == nullptr;
                        properties[QString("×anchor%1").arg(i)] = anchors[i];
                    }
                }

            }
        } else {
            qDebug() << reply->errorString() << reply->readAll();
            if (reply->error() != QNetworkReply::ContentNotFoundError) {
                Segmentation::singleton().clearObjectSelection();
                Skeletonizer::singleton().select<treeListElement>({});
                return;
            }
        }
        selectTrees(soid, properties);

        auto & obj = seg.objects[seg.largestObjectContainingSubobjectId(soid, pos)];
        qDebug() << "mergelist" << obj.id << obj.subobjects.size() << "subobjects";
        seg.clearObjectSelection();

        obj.immutable = true;

        Skeletonizer::singleton().resetData();
        seg.selectObject(obj);
        bs.unblock();
        seg.resetData();

        qDebug() << seg.selectedObjectIndices.size() << seg.selectedObjectIndices.front()
                 << seg.objects[seg.selectedObjectIndices.front()].id
                 << seg.objects[seg.selectedObjectIndices.front()].subobjects.size()
                 << seg.objects.front().id
                 << seg.objects.front().subobjects.size();

        QStringList slist;
        std::vector<std::uint64_t> svec;
        for (const auto & so : obj.subobjects) {
            if (auto * tree = Skeletonizer::singleton().findTreeByTreeID(so.get().id); !tree || !tree->mesh) {// filter out existing trees/meshes
                svec.emplace_back(so.get().id);
                slist << QString::number(svec.back());
            }
        }
        if (svec.empty()) {
            return;
        }

        const int preflod = state->mainWindow->widgetContainer.annotationWidget.segmentationTab.lodSpin.value();
        const int maxlod{std::max(preflod, recomlod)};
        static void (*lmbd)(int, QProgressBar&, int, QUrl, decltype(slist), decltype(svec), QElapsedTimer) = [](int lod, auto &downloadProgress, auto maxlod, auto url, auto slist, auto svec, const auto timer) -> void {
            downloadProgress.setMaximum(0);
            downloadProgress.show();
            if (!url.isValid()) {
                throw std::runtime_error("missing _MeshServer");
            }
            QNetworkRequest request{url.toString() + "/meshes"};
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            const auto payload = QString{R"json({"supervoxelIds": [%1], "lod": %2})json"}.arg(slist.join(',')).arg(lod).toUtf8();
            QElapsedTimer ltime;
            ltime.start();
            auto * const reply = Network::singleton().manager.post(request, payload);
            QObject::connect(reply, &QNetworkReply::downloadProgress, &downloadProgress, [&downloadProgress, lodi=maxlod - lod, lodcount=1+std::abs(maxlod - maxlod)](auto val, auto max){
                downloadProgress.setMaximum(max * lodcount);
                downloadProgress.setValue(val + (max * lodi));
            });
            QObject::connect(reply, &QNetworkReply::uploadProgress, [size=payload.size(), ltime](auto val, auto max){
                if (val > 0 &&  val == max) {
                    qDebug() << size / 1e3 << "k uploaded after" << ltime.nsecsElapsed()/1e6 << "ms";
                }
            });
            QObject::connect(reply, &QNetworkReply::finished, [reply, lod, maxlod, svec, &downloadProgress, &addProgress = state->mainWindow->meshAddProgressBar, timer, ltime, url, slist]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::OperationCanceledError) {
                    qDebug() << "mesh download canceled";
                }
                if (lod == maxlod) {
                    QTimer::singleShot(450, &downloadProgress, &QProgressBar::hide);
                }
                qDebug() << lod << "∊" << maxlod << ".." << maxlod << reply->size() / 1e6 << "M in" << ltime.nsecsElapsed()/1e9 << "s";
                if (reply->error() == QNetworkReply::NoError) {
                    const auto jarray = QJsonDocument::fromJson(reply->readAll())["meshes"].toArray();
                    QElapsedTimer time;
                    time.start();
                    {QSignalBlocker bs2{Skeletonizer::singleton()};
                    addProgress.setMaximum(svec.size());
                    addProgress.setValue(0);
                    addProgress.show();
                    for (const auto & [ply, so] : boost::combine(jarray, svec)) {
                        addProgress.setValue(addProgress.value() + 1);
                        QElapsedTimer timex;
                        timex.start();
                        auto b64d = QByteArray::fromBase64Encoding(ply.toString().toUtf8());
                        auto b = QBuffer{&b64d.decoded};
                        Skeletonizer::singleton().loadMesh(b, so, "dataset mesh");
                    }
                    QTimer::singleShot(450, &addProgress, &QProgressBar::hide);
                    qDebug() << time.nsecsElapsed()/1e9 << "s mesh add →" << timer.nsecsElapsed()/1e9 << "s";
                    }Skeletonizer::singleton().resetData();
                } else {
                    qDebug() << reply->error() << reply->errorString() << reply->readAll();
                }

                if (lod > maxlod) {
                    lmbd(lod - 1, downloadProgress, maxlod, url, slist, svec, timer);
                }
            });
        };
        lmbd(maxlod, downloadProgress, maxlod, dataset.meshServer, slist, svec, timer);

        if (anchorLoadedForTheFirstTime && anchors.size() > 1) {
            QMessageBox box{QApplication::activeWindow()};
            box.setIcon(QMessageBox::Warning);
            box.setText(QObject::tr("Object contains multiple anchors."));
            box.setInformativeText(QObject::tr("Choose »Select Anchor object(s)« in the tree table to show the anchors in the 3D viewport."));
            box.exec();
        }
    });
}

template<bool isDirectConnectivity>
void fetchFragmentsSwitch(const Dataset & dataset, const std::uint64_t soid, QElapsedTimer & timer, const int assert) {
    auto * reply = fragmentListRequest<isDirectConnectivity>(dataset, soid);
    QObject::connect(&Network::singleton(), &Network::abortRequest, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, [reply, timer, soid, &dataset, assert, &downloadProgress = state->mainWindow->meshDownloadProgressBar, &addProgress = state->mainWindow->meshAddProgressBar]() mutable {
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            qDebug() << "aborted fragmentListRequest";
            resetBusyIfMatch(soid);
            reply->deleteLater();
            return;
        }
        const auto data = reply->readAll();
        auto responseObject = QJsonDocument::fromJson(data).object();
        auto fragidsv = responseObject["fragmentKey"].toArray().toVariantList();
        auto soidsv = responseObject["supervoxelId"].toArray().toVariantList();
        auto anchorsObj = responseObject.value("anchor").toObject();
        auto timestampValue = responseObject.value("timestamp");
        auto user = responseObject.value("user");
        std::optional<QString> timestamp;
        QString note;
        std::optional<quint64> nuc;
        std::vector<quint64> anchors;
        if (!anchorsObj.empty()) {
            auto nucValue = anchorsObj.begin().value().toObject().value("nuc");
            if (!nucValue.isUndefined()) {
                nuc = nucValue.toVariant().toULongLong();
            }
            note = anchorsObj.begin().value().toObject().value("note").toString();
            for (auto iter = anchorsObj.begin(); iter != anchorsObj.end(); iter++) {
                anchors.push_back(iter.key().toULongLong());
            }
        }
        if (!user.isUndefined() && !user.isNull()) {
            timestamp = QString("%1 %2").arg(QDateTime::fromSecsSinceEpoch(timestampValue.toVariant().toULongLong()).toString("yyyy-MM-ddThh:mm"), user.toString());
        }
        QSet<quint64> cc;
        for (const auto & so : soidsv) {
            cc.insert(so.toULongLong());
        }
        const auto error = QString("%1 %2 %3").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
              .arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString())
              .arg(reply->errorString());
        qDebug() << "received" << fragidsv.size() << "fragements in" << cc.size() << "subobjects for" << soid << reply->error()
                 << (reply->error() != QNetworkReply::NoError ? error.toUtf8().constData() : "in") << timer.elapsed() << "ms";
        // create mergelist and trees
        auto & seg = Segmentation::singleton();
        QSignalBlocker bs{seg};
        bool anchorLoadedForTheFirstTime = true;
        if (!isDirectConnectivity) {
            resetBusyIfMatch(soid);
            QSet<quint64> prevIds;
            if (assert && seg.hasObjects()) {
                for (const auto & so : seg.objects.front().subobjects) {
                    prevIds.insert(so.get().id);
                }
            }
            const auto bak = Segmentation::singleton().hovered_subobject_id;// drag
            seg.mergelistClear();
            Segmentation::singleton().hovered_subobject_id = bak;
            auto & obj = seg.hasObjects() ? seg.objects.front() : seg.createObject();
            seg.unselectObject(obj);
            obj.immutable = true;
            QSet<quint64> uniqueIds;
            {
                decltype(treeListElement::properties) properties;
                if (!anchors.empty()) {
                    properties["anchor"] = anchors[0];
                    if (nuc.has_value()) {
                        properties["nuc"] = nuc.value();
                    }
                    if (!note.isEmpty()) {
                        properties["note"] = note;
                    }
                }
                if (timestamp.has_value()) {
                    properties["t"] = timestamp.value();
                }
                if (anchors.size() > 1) {
                    for (std::size_t i = 1; i < anchors.size(); i++) {
                        anchorLoadedForTheFirstTime = Skeletonizer::singleton().findTreeByTreeID(anchors[i]) == nullptr;
                        properties[QString("×anchor%1").arg(i)] = anchors[i];
                    }
                }

                QSignalBlocker bs2{Skeletonizer::singleton()};
                QSet<treeListElement*> treesToSelect;
                for (int i = 0; i < soidsv.size(); ++i) {
                    const auto soid = soidsv[i].toULongLong();
                    uniqueIds.insert(soid);
                    if (auto tree = Skeletonizer::singleton().findTreeByTreeID(soid)) {// filter out existing trees/meshes
                        treesToSelect.insert(tree);
                        fragidsv.removeAt(i);
                        soidsv.removeAt(i);
                        --i;
                    }
                }
                for (const auto soid : uniqueIds) {
                    if (!seg.subobjectExists(soid)) {
                        seg.newSubObject(obj, soid);
                    }
                    auto tree = Skeletonizer::singleton().findTreeByTreeID(soid);
                    if (tree == nullptr) {
                        treesToSelect.insert(&Skeletonizer::singleton().addTree(soid));
                    }
                }
                for (auto & elem : Skeletonizer::singleton().skeletonState.selectedTrees) {
                    elem->selected = false;
                }
                Skeletonizer::singleton().skeletonState.selectedTrees.clear();
                for (auto * tree : treesToSelect) {
                    tree->properties = properties;
                    Skeletonizer::singleton().skeletonState.selectedTrees.emplace_back(tree);
                    tree->selected = true;
                }
            }
            Skeletonizer::singleton().resetData();
            seg.selectObject(obj);
            bs.unblock();
            seg.resetData();

            if (assert && uniqueIds == prevIds) {
                qDebug() << "subobject list didn’t change –" << prevIds.size() << "vs." << uniqueIds.size() << "– requesting again, " << assert << " tries remaining";
                retrieveMeshes(soid, assert - 1);
                return;
            }
        }
        static download_list download_data;
        if (!fragidsv.empty() && dataset.api == Dataset::API::GoogleBrainmaps) {
            const auto chunk_size = std::max(1, std::min(256, fragidsv.size() / 6));
            for (int c = 0; c < fragidsv.size(); c += chunk_size) {
                if (!isDirectConnectivity && abortMeshDownload) {
                    qDebug() << "not starting mesh download. currently at" << c << "of" << fragidsv.size();
                    break;
                }
                QJsonObject request;
                QJsonArray batches;
                for (int i = c; i < std::min(c + chunk_size, fragidsv.size()); ++i) {
                    QJsonObject frags;
                    frags["objectId"] = soidsv[i].toString();
                    frags["fragmentKeys"] = fragidsv[i].toString();
                    batches.append(frags);
                }
                request["volumeId"] = dataset.url.toString().section('/', 5, 5);
                request["meshName"] = dataset.brainmapsMeshKey;
                request["batches"] = batches;

                auto * reply = googleRequestAsync(dataset.token, QUrl{"https://brainmaps.googleapis.com/v1/objects/meshes:batch"}, QJsonDocument{request}.toJson());
                download_data[soid].replies.emplace(reply);

                downloadProgress.setMaximum(downloadProgress.maximum() + 1);
                downloadProgress.show();
                if (!isDirectConnectivity) {
                    QObject::connect(&Network::singleton(), &Network::abortRequest, reply, &QNetworkReply::abort);
                }
                QObject::connect(reply, &QNetworkReply::finished, [reply, timer, soid, &downloadProgress, &addProgress]() mutable {
                    downloadProgress.setValue(downloadProgress.value() + 1);
                    if (reply->error() == QNetworkReply::OperationCanceledError) {
                        qDebug() << "mesh download canceled";
                    }
                    if (downloadProgress.value() == downloadProgress.maximum()) {
                        downloadProgress.setValue(0);
                        downloadProgress.setMaximum(0);
                    }
                    if (reply->error() == QNetworkReply::NoError) {
                        download_data[soid].size += reply->size();
                        parseBinaryMesh(*reply, download_data[soid].mesh_data);

                    }
                    download_data[soid].replies.erase(reply);
                    if (download_data[soid].replies.empty()) {
                        updateAnnotation(download_data, timer, soid, downloadProgress, addProgress);
                    }

                    reply->deleteLater();
                });
            }
        }
        if (anchorLoadedForTheFirstTime && anchors.size() > 1) {
            QMessageBox box{QApplication::activeWindow()};
            box.setIcon(QMessageBox::Warning);
            box.setText(QObject::tr("Object contains multiple anchors."));
            box.setInformativeText(QObject::tr("Choose »Select Anchor object(s)« in the tree table to show the anchors in the 3D viewport."));
            box.exec();
        }
        reply->deleteLater();
    });
}

static auto fetchFragments = fetchFragmentsSwitch<false>;

void fetchDCFragments(const Dataset & dataset, const std::uint64_t soid) {
    static QElapsedTimer timer;
    timer.start();

    const QUrl url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString{"/%1/equivalences:list"}.arg(dataset.brainmapsChangeStack);
    const auto payload = QString{R"json({"segmentId": ["%1"]})json"}.arg(soid).toUtf8();
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto * const reply = dataset.useAlternativebrainmapsChangeServer ? Network::singleton().manager.post(request, payload) : googleRequestAsync(dataset.token, url, payload);
    QObject::connect(reply, &QNetworkReply::finished, [reply, soid, dataset](){
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "dc error" << reply->readAll();
        }
        const auto edgeJson = QJsonDocument::fromJson(reply->readAll());
        QSet<quint64> ids;
        for (const auto edge : edgeJson["edge"].toArray()) {
            ids.insert(edge.toObject()["first"].toString().toULongLong());
            ids.insert(edge.toObject()["second"].toString().toULongLong());
        }
        if (ids.empty()) {
            ids.insert(soid);
        }
        qDebug() << soid << "list" << /*edgeJson << ids << */ids.size() << timer.elapsed() << "ms";
        auto & seg = Segmentation::singleton();

        QSignalBlocker bs{seg};
        const auto bak = Segmentation::singleton().hovered_subobject_id;// drag
        seg.mergelistClear();
        Segmentation::singleton().hovered_subobject_id = bak;
        auto & obj = seg.createObject();
        seg.unselectObject(obj);
        obj.immutable = true;
        {
            QSignalBlocker bs2{Skeletonizer::singleton()};
            for (auto & elem : Skeletonizer::singleton().skeletonState.selectedTrees) {
                elem->selected = false;
            }
            Skeletonizer::singleton().skeletonState.selectedTrees.clear();
            for (const auto soid : ids) {
                if (!seg.subobjectExists(soid)) {
                    seg.newSubObject(obj, soid);
                }
                auto tree = Skeletonizer::singleton().findTreeByTreeID(soid);
                if (tree == nullptr) {
                    tree = &Skeletonizer::singleton().addTree(soid);
                    fetchFragmentsSwitch<true>(dataset, soid, timer, 0);
                }
                Skeletonizer::singleton().skeletonState.selectedTrees.emplace_back(tree);
                tree->selected = true;
            }
        }
        Skeletonizer::singleton().resetData();
        seg.selectObject(obj);
        bs.unblock();
        seg.resetData();
        resetBusyIfMatch(soid);// this download is done
    });
}

void retrieveMeshes(const std::uint64_t soid, const int assert) {
    if (abortMeshDownload) {
        qDebug() << "abort retrieveMeshes";
        return;
    }
    QElapsedTimer timer;
    timer.start();

    brainmapsDownload = soid;
    state->viewer->reslice_notify(Segmentation::singleton().layerId);
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    if (dataset.aggloServer.isValid()) {
        fetchAgglo(dataset, soid, timer, Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit));
    } else {
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit)) {
            fetchDCFragments(dataset, soid);
        } else {
            fetchFragments(dataset, soid, timer, assert);
        }
    }
}

#include <QProgressDialog>

struct Action {
    enum Type { None, Merge, Split };
    Type type;
    std::uint64_t src_soid;
    std::uint64_t dst_soid;
};

Action lastAction;
bool bmUndoable() {
    return lastAction.type != Action::Type::None;
}

void bmmergesplit(const std::uint64_t src_soid, const std::uint64_t dst_soid, const bool undo) {
    const auto shift = Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit);
    if (!undo && bminvalid(shift, src_soid, dst_soid)) {
        return;
    }
    auto * focusedWidget = qApp->focusWidget(); // vp loses focus on mainwindow disable, bad for space key
    state->mainWindow->setEnabled(false);
    QEventLoop pause;
    auto wait = [&pause](){
        while (brainmapsDownload) {
            QTimer::singleShot(10, nullptr, [&pause](){
                pause.exit();
            });
            pause.exec();
        }
    };
    wait();
    Annotation::singleton().annotationMode.setFlag(AnnotationMode::ObjectMerge, false);
    Annotation::singleton().annotationMode.setFlag(AnnotationMode::SubObjectSplit, false);
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    if (shift && !dataset.useAlternativebrainmapsChangeServer) {// retrieve prior cc merge list first
        retrieveMeshes(src_soid);
        wait();
    }
    if (!undo && bminvalid(shift, src_soid, dst_soid)) {
        state->mainWindow->setEnabled(true);
        return;
    }
    brainmapsDownload = src_soid;// try to keep showing yellow
    state->viewer->segmentation_changed();

    QElapsedTimer time;
    time.start();
    QPair<bool, QByteArray> pair;
    QProgressDialog progress{"Waiting for the server to process the request", "abort", 0, 0};
    //progress.setMinimumDuration(500);
    //progress.setModal(true);
    for (int retry{10}; retry > 0; --retry) {
        const auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString("/%1/equivalences:%2")
                .arg(dataset.brainmapsChangeStack).arg(shift ? "delete" : "set");
        const auto payload = QString{R"json({"edge": {"first": %1, "second": %2}%9})json"}.arg(src_soid).arg(dst_soid)
                .arg(shift ? "" : R"json(,"allowEquivalencesToBackground": false)json").toUtf8();
        qDebug() << url << payload;
        QElapsedTimer time;
        time.start();
        QNetworkRequest request{url};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        pair = dataset.useAlternativebrainmapsChangeServer ? blockDownloadExtractData(*Network::singleton().manager.post(request, payload)) : googleRequestBlocking(dataset.token, url, payload);
        qDebug() << pair.first << time.nsecsElapsed()/1e9;
        if (pair.first || progress.wasCanceled() || dataset.useAlternativebrainmapsChangeServer) {
            progress.cancel();
            break;
        }
        if (!pair.first) {
            QTimer::singleShot(5, [&pause](){
                pause.exit();
            });
            pause.exec();
        }
    }
    state->mainWindow->setEnabled(true);
    state->mainWindow->activateWindow(); // can’t focus if window not active
    if (focusedWidget != nullptr) {
        focusedWidget->setFocus();
    }
    if (!pair.first) {
        QMessageBox box{QApplication::activeWindow()};
        box.setIcon(QMessageBox::Warning);
        box.setText(QObject::tr("Failed to upload changes."));
        if (!dataset.useAlternativebrainmapsChangeServer) {
            box.setInformativeText(QObject::tr("They may have still been applied, but the server only responded with errors."));
        }
        box.setDetailedText(pair.second.data());
        box.exec();
        qDebug() << box.text() << box.informativeText() << pair.second.data();
        if (dataset.useAlternativebrainmapsChangeServer) { // if permission error, there’s nothing to download
            brainmapsDownload = boost::none;
            state->viewer->segmentation_changed();
        }
    } else {
        qDebug() << "equivalences" << pair.second.data() << time.nsecsElapsed()/1e9 << "s";
        lastAction = {shift ? Action::Type::Split : Action::Type::Merge, src_soid, dst_soid};
        if (undo) {
            lastAction.type = Action::Type::None;
            abortMeshDownload = false;
        }
        retrieveMeshes(src_soid, 20 * !dataset.useAlternativebrainmapsChangeServer);
    }
}

void bmundo() {
    if (!bmUndoable()) {
        return;
    }
    qDebug() << "UNDO";
    emit Network::singleton().abortRequest();
    abortMeshDownload = true;
    Annotation::singleton().annotationMode.setFlag((lastAction.type == Action::Type::Merge) ? AnnotationMode::SubObjectSplit
                                                                                            : AnnotationMode::ObjectMerge);
    lastAction.type = Action::Type::None;

    bmmergesplit(lastAction.src_soid, lastAction.dst_soid, true);
    abortMeshDownload = false;
    {
        QSignalBlocker blocker{Skeletonizer::singleton()};
        for (auto rIt = std::rbegin(Skeletonizer::singleton().skeletonState.trees); rIt != std::rend(Skeletonizer::singleton().skeletonState.trees); ) {
            if (rIt->mesh == nullptr) {
                Skeletonizer::singleton().delTree(rIt->treeID);
            } else {
                ++rIt;
            }
        }
    }
    emit Skeletonizer::singleton().resetData();
}

#include <unordered_set>
std::unordered_set<std::uint64_t> split0ids;
std::unordered_set<std::uint64_t> split1ids;

Segmentation::color_t Segmentation::brainmapsColor(const std::uint64_t subobjectId, const bool selected) const {
    Segmentation::color_t color;
    if (brainmapsDownload) {// busy
        color = std::make_tuple(255, 255, 0, Segmentation::singleton().alpha);
    } else if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit)) {
        color = Segmentation::singleton().subobjectColor(subobjectId);
        if (subobjectId == Segmentation::singleton().srcSvx) {
            color = std::make_tuple(255, 0, 0, Segmentation::singleton().alpha);// red
        }
    } else {
        color = std::make_tuple(0, 255, 0, Segmentation::singleton().alpha);// green
        if (split0ids.find(subobjectId) != std::end(split0ids)) {
            const QColor blue = Qt::blue;
            color = {blue.red(), blue.green(), blue.blue(), Segmentation::singleton().alpha};
        } else if (split1ids.find(subobjectId) != std::end(split1ids)) {
            const QColor darkMagenta = Qt::darkMagenta;
            color = {darkMagenta.red(), darkMagenta.green(), darkMagenta.blue(), Segmentation::singleton().alpha};
        }
    }
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::ObjectMerge)) {
        if (!selected) {
            color = Segmentation::singleton().colorObjectFromSubobjectId(subobjectId);
        }
    } else if (!selected) {
        if (subobjectId == Segmentation::singleton().getBackgroundId() || Segmentation::singleton().bmRenderOnlySelectedObjs) {
            color = {};
        } else {
            color = Segmentation::singleton().subobjectColor(subobjectId);
        }
    }
    return color;
}


QColor brainmapsMeshColor(const decltype(treeListElement::treeID) treeID) {
    if (split0ids.find(treeID) != std::end(split0ids)) {
        return Qt::blue;
    }
    if (split1ids.find(treeID) != std::end(split0ids)) {
        return Qt::darkMagenta;
    }
    QColor color;
    auto objColor = Segmentation::singleton().colorOfSelectedObject();
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit)) {
        objColor = Segmentation::singleton().subobjectColor(treeID);
    }
    color = std::apply(static_cast<QColor(*)(int,int,int,int)>(QColor::fromRgb), objColor);
    color = QColor::fromRgb(std::get<0>(objColor), std::get<1>(objColor), std::get<2>(objColor));// skip seg alpha
    if (!Annotation::singleton().annotationMode.testFlag(AnnotationMode::SubObjectSplit)) {
        color = Qt::green;
    } else if (treeID == Segmentation::singleton().srcSvx) {
        color = Qt::black;
    }
    const bool meshing = state->mainWindow->meshDownloadProgressBar.value() != state->mainWindow->meshDownloadProgressBar.maximum() || state->mainWindow->meshAddProgressBar.value() != state->mainWindow->meshAddProgressBar.maximum();
    if (brainmapsDownload || meshing) {// busy
        color =  Qt::yellow;
    }
    return color;
}

void brainmapsBranchPoint(const boost::optional<nodeListElement &> node, std::uint64_t subobjID, const Coordinate & globalCoord) {
    nodeListElement * pushBranchNode = nullptr;
    if (node) {
        pushBranchNode = &node.get();
    } else {
        if (auto * tree = Skeletonizer::singleton().findTreeByTreeID(subobjID); tree && tree->selected) {
            pushBranchNode = &Skeletonizer::singleton().addNode(boost::none, 20, tree->treeID, globalCoord, ViewportType::VIEWPORT_UNDEFINED, -1, boost::none, false, {}).get();
            state->skeletonState->activeTree = tree;
            state->skeletonState->activeNode = pushBranchNode;
        }
    }
    if (pushBranchNode) {
        Skeletonizer::singleton().pushBranchNode(*pushBranchNode);
    }
}

Coordinate p0, p1;
std::uint64_t soid{};
std::size_t segment_index{0};

void setSplit(const Coordinate & p, const std::uint64_t id) {
    p1 = p0;// shift
    p0 = p;
    soid = id;
    segment_index = 0;
}

auto retrieveGraph(std::uint64_t soid) {
    const auto dataset = Dataset::datasets[Segmentation::singleton().layerId];
    {
//    POST https://brainmaps.googleapis.com/v1/changes/{header.volumeId}/{header.changeStackId}/equivalences:getgroups
    const auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString{"/%1/equivalences:getgroups"}.arg(dataset.brainmapsChangeStack);
    const auto payload = QString{R"json({"segmentId": ["%1"]})json"}.arg(soid).toUtf8();
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const auto pair = dataset.useAlternativebrainmapsChangeServer ? blockDownloadExtractData(*Network::singleton().manager.post(request, payload)) : googleRequestBlocking(dataset.token, url, payload);
    if (!pair.first) {
        throw std::runtime_error("getgroups fail");
    }
    const auto json{QJsonDocument::fromJson({pair.second})};
    const auto group{json["groups"][0]["groupMembers"]};
    qDebug() << group.toArray().size()/* << group*/;
    QJsonObject o;
    o["segmentId"] = group;
    {
    const auto url = dataset.brainmapsChangeServer.toString().replace("volumes", "changes") + QString{"/%1/equivalences:list"}.arg(dataset.brainmapsChangeStack);
    const auto payload = QJsonDocument{o}.toJson();
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const auto pair = dataset.useAlternativebrainmapsChangeServer ? blockDownloadExtractData(*Network::singleton().manager.post(request, payload)) : googleRequestBlocking(dataset.token, url, payload);
    if (!pair.first) {
        throw std::runtime_error("list fail");
    }
    auto life = std::make_unique<Skeletonizer>();
    auto & graph = *life;
    for (auto edge : QJsonDocument::fromJson({pair.second})["edge"].toArray()) {
        // const auto n0id = edge.toObject()["first"].toString().toULongLong();
        // const auto n1id = edge.toObject()["second"].toString().toULongLong();
        // auto x = [&dataset, &graph](auto nid){
        //     auto * n = graph.findNodeByNodeID(nid);
        //     if (!n) {
        //         auto * tree = Skeletonizer::singleton().findTreeByTreeID(nid);
        //         auto pos = floatCoordinate{};
        //         tree->mesh->position_buf.bind();
        //         tree->mesh->position_buf.read(0, &pos, 3 * sizeof(float));
        //         tree->mesh->position_buf.release();
        //         pos /= dataset.scales[0];

        //         tree = graph.findTreeByTreeID(nid);
        //         if (!tree) {
        //             tree = &graph.addTree(nid);
        //         }
        //         return &graph.addNode(nid, pos, *tree).get();
        //     }
        //     return n;
        // };
        // graph.addSegment(*x(n0id), *x(n1id));
    }
    return life;
    }
    }
}

#include <QOpenGLContext>

void splitMe() {
    if (!split0ids.empty() && !split1ids.empty()) {
        split0ids.clear();
        split1ids.clear();
        return;
    }
    if (soid == Segmentation::singleton().backgroundId) {
        return;
    }
    auto life = retrieveGraph(soid);
    auto & graph = *life;

    static QOffscreenSurface s;
    if (!s.isValid()) {
        s.create();
    }
    qDebug() << QOpenGLContext::globalShareContext()->makeCurrent(&s);

    const auto dataset = Dataset::datasets[Segmentation::singleton().layerId];
    auto findNearbyNodeOfTreeByVertex = [&dataset, &graph](Coordinate ref) -> nodeListElement & {
        Coordinate min;
        treeListElement * minTree{nullptr};
        for (auto & tree : state->skeletonState->trees) {
            if (!tree.mesh || !tree.mesh->vertex_count || !Mesh::unibuf.bind()) {
                qDebug() << "tree unsuitable for splitting" << tree.treeID << tree.mesh->vertex_count;
                continue;
            }
            QVector<floatCoordinate> vertices(tree.mesh->vertex_count);
            if (Mesh::unibuf.read(*tree.mesh->position_buf, vertices.data(), vertices.size() * sizeof (vertices[0]))) {
                qDebug() << "read" << tree.treeID << tree.mesh->vertex_count;
                for (auto & pos : vertices) {
                    pos /= dataset.scales[0];
                    if ((ref - pos).length() < (ref - min).length()) {
                        min = pos;
                        minTree = &tree;
                    }
                }
            } else {
                qDebug() << "no read" << tree.treeID << tree.mesh->vertex_count;
            }
            Mesh::unibuf.release();
        }
        if (!minTree) {
            throw std::runtime_error{"no min tree for splitting"};
        }
        return *graph.findNearbyNode(graph.findTreeByTreeID(minTree->treeID), ref);
    };

    std::vector<nodeListElement *> shortestPathV(nodeListElement & lhs, const nodeListElement & rhs);
    auto path = shortestPathV(findNearbyNodeOfTreeByVertex(p0), findNearbyNodeOfTreeByVertex(p1));
    split0ids.clear();
    split1ids.clear();
    qDebug() << "path.size()" << path.size();
    if (path.size() == 1) {
        qDebug() << "already only 1 object – nothing to split";
        return;
    }
    std::ptrdiff_t totalSize{0};
    for (std::size_t i{0}; i < path.size(); ++i) {
        totalSize += Skeletonizer::singleton().findTreeByTreeID(path[i]->correspondingTree->treeID)->mesh->vertex_count;
    }
    std::ptrdiff_t rollingSize{0}, min{totalSize};
    bool done{false};
    for (std::size_t i{0}; i < path.size(); ++i) {
        const auto inc{Skeletonizer::singleton().findTreeByTreeID(path[i]->correspondingTree->treeID)->mesh->vertex_count};
        const auto diff{std::abs(static_cast<std::ptrdiff_t>((rollingSize + inc) - totalSize/2))};
        if (!done && diff < min) {// getting closer to our splitting point
            rollingSize += inc;
            min = diff;
            split0ids.insert(path[i]->correspondingTree->treeID);
        } else {
            done = true;
            split1ids.insert(path[i]->correspondingTree->treeID);
        }
    }
    qDebug() << rollingSize << totalSize << min << split0ids.size() << split1ids.size();
}

#include <skeleton/skeleton_dfs.h>

void splitMe2() {
    if (soid == Segmentation::singleton().backgroundId) {
        return;
    }
    auto life = retrieveGraph(soid);
    auto & graph = *life;
    if (graph.skeletonState.nodesByNodeID.empty()) {
        return;
    }

    auto & node = *graph.findNodeByNodeID(soid);
    split0ids.clear();
    split1ids.clear();
    node.segments.sort([&node](const auto & lhs, const auto & rhs){
        return (lhs.source == node ? lhs.target.nodeID : lhs.source.nodeID) < (rhs.source == node ? rhs.target.nodeID : rhs.source.nodeID);
    });
    std::cout << node.segments.size();
    for (auto segmentIt = std::begin(node.segments); segmentIt != std::end(node.segments); ++segmentIt) {
        std::cout << "  " << segmentIt->source.nodeID << ' ' << segmentIt->target.nodeID;
    }
    std::cout << std::endl;
    if (segment_index >= node.segments.size()) {
        segment_index = 0;
        return;
    }
    if (segment_index < node.segments.size()) {
        auto & seg = *std::next(std::begin(node.segments), segment_index);
        const auto targetid = seg.forward ? seg.target.nodeID : seg.source.nodeID;
        for (auto segmentIt = std::begin(node.segments); segmentIt != std::end(node.segments);) {
            if (segmentIt->target.nodeID != targetid && segmentIt->source.nodeID != targetid) {
                graph.delSegment(segmentIt);
                segmentIt = std::begin(node.segments);
            } else {
                ++segmentIt;
            }
        }
    }
    ++segment_index;
    for (auto it = NodeGenerator(node, NodeGenerator::Direction::Any); !it.reachedEnd; ++it) {
        split0ids.insert((*it).correspondingTree->treeID);
    }
}

void splitHightlightToSelection() {
    if (split0ids.empty() || !split1ids.empty()) {
        return;
    }
    QSet<treeListElement *> treeSelection;
    for (auto treeId : split0ids) {
        treeSelection.insert(Skeletonizer::singleton().findTreeByTreeID(treeId));
    }
    Skeletonizer::singleton().select(treeSelection);
    split0ids.clear();
}

void agglo_lock(const std::size_t soid) {
    const auto & dataset = Dataset::datasets[Segmentation::singleton().layerId];
    const auto res = Network::singleton().refresh(dataset.aggloServer.toString() + QString{"/lock/%1"}.arg(soid));
    QMessageBox info{qApp->activeWindow()};
    info.setIcon(QMessageBox::Information);
    info.setText(res.second);
    info.exec();
}
