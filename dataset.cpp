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

#include "dataset.h"

#include "network.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QUrlQuery>

#include <boost/assign.hpp>
#include <boost/bimap.hpp>


Dataset::list_t Dataset::datasets;

static boost::bimap<QString, Dataset::CubeType> typeMap = boost::assign::list_of<decltype(typeMap)::relation>
        (".raw", Dataset::CubeType::RAW_UNCOMPRESSED)
        (".png", Dataset::CubeType::RAW_PNG)
        (".jpg", Dataset::CubeType::RAW_JPG)
        (".j2k", Dataset::CubeType::RAW_J2K)
        (".6.jp2", Dataset::CubeType::RAW_JP2_6)
        (".seg.sz.zip", Dataset::CubeType::SEGMENTATION_SZ_ZIP)
        (".seg", Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64);

QString Dataset::compressionString() const {
    switch (type) {
    case Dataset::CubeType::RAW_UNCOMPRESSED: return "8 bit gray";
    case Dataset::CubeType::RAW_JPG: return "jpg";
    case Dataset::CubeType::RAW_J2K: return "j2k";
    case Dataset::CubeType::RAW_JP2_6: return "jp2";
    case Dataset::CubeType::RAW_PNG: return "png";
    case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_16: return "16 bit id";
    case Dataset::CubeType::SEGMENTATION_UNCOMPRESSED_64: return "64 bit id";
    case Dataset::CubeType::SEGMENTATION_SZ_ZIP: return "seg.sz.zip";
    case Dataset::CubeType::SNAPPY: return "snappy";
    }
    throw std::runtime_error(QObject::tr("no compressions string for %1").arg(static_cast<int>(type)).toUtf8()); ;
}

QString Dataset::apiString() const {
    switch(api) {
    case Dataset::API::GoogleBrainmaps:
        return "GoogleBrainmaps";
    case Dataset::API::Heidelbrain:
        return "Heidelbrain";
    case Dataset::API::OpenConnectome:
        return "OpenConnectome";
    case Dataset::API::PyKnossos:
        return "PyKnossos";
    case Dataset::API::WebKnossos:
        return "WebKnossos";
    }
    throw std::runtime_error(QObject::tr("no api string for %1").arg(static_cast<int>(type)).toUtf8()); ;
}

bool Dataset::isGoogleBrainmaps(const QUrl & url) {
    return url.toString().contains("google");
}

bool Dataset::isHeidelbrain(const QUrl & url) {
    return !isGoogleBrainmaps(url) && !isNeuroDataStore(url) && !isPyKnossos(url) && !isWebKnossos(url) && !isToml(url);
}

bool Dataset::isNeuroDataStore(const QUrl & url) {
    return url.path().contains("/nd/sd/") || url.path().contains("/ocp/ca/");
}

bool Dataset::isPyKnossos(const QUrl & url) {
    return url.path().endsWith("ariadne.conf") || url.path().endsWith(".pyknossos.conf") || url.path().endsWith(".pyk.conf") || url.path().endsWith(".pyk.auth.conf");
}

bool Dataset::isToml(const QUrl & url) {
    return url.path().endsWith(".k.toml");
}

bool Dataset::isWebKnossos(const QUrl & url) {
    return url.host().contains("webknossos");
}

Dataset::list_t Dataset::parse(const QUrl & url, const QString & data, bool add_snappy) {
    Dataset::list_t infos;
    if (Dataset::isWebKnossos(url)) {
        infos = Dataset::parseWebKnossosJson(url, data);
    } else if (Dataset::isGoogleBrainmaps(url)) {
        infos = Dataset::parseGoogleJson(url, data);
    } else if (Dataset::isNeuroDataStore(url)) {
        infos = Dataset::parseNeuroDataStoreJson(url, data);
    } else if (Dataset::isPyKnossos(url)) {
        infos = Dataset::parsePyKnossosConf(url, data);
    } else if (Dataset::isToml(url)) {
        infos = Dataset::parseToml(url, data);
    } else {
        infos = Dataset::fromLegacyConf(url, data);
    }
    if (infos.empty()) {
        throw std::runtime_error("Missing [Dataset] header in config.");
    }
    if (add_snappy) {
        bool overlayPresent{false};
        for (std::size_t i = 0; i < infos.size(); ++i) {// determine segmentation layer
            if (infos[i].isOverlay()) {
                overlayPresent = true;
                infos[i].allocationEnabled = infos[i].loadingEnabled = infos[i].renderSettings.visible = true;
                break;// only enable the first overlay layer by default
            }
        }
        if (!overlayPresent) {// add empty overlay channel
            const auto i = infos.size();
            infos.emplace_back(Dataset{infos.front()});// explicitly copy, because the reference will get invalidated
            infos[i].renderSettings = {};
            infos[i].allocationEnabled = infos[i].loadingEnabled = infos[i].renderSettings.visible = true;
            infos.back().type = Dataset::CubeType::SNAPPY;
        }
    }
    return infos;
}

Dataset::list_t Dataset::parseGoogleJson(const QUrl & infoUrl, const QString & json_raw) {
    Dataset info;
    info.api = API::GoogleBrainmaps;
    info.experimentname = QFileInfo{infoUrl.path()}.fileName().section(':', 2);
    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();

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
    info.scale = info.scales.front();

    info.magIndex = 0;
    info.lowestAvailableMagIndex = 0;
    info.highestAvailableMagIndex = jmap["geometry"].toArray().size() - 1; //highest google mag
    info.type = CubeType::RAW_JPG;

    info.url = infoUrl;

    return {info};
}

Dataset::list_t Dataset::parseNeuroDataStoreJson(const QUrl & infoUrl, const QString & json_raw) {
    Dataset info;
    info.api = API::OpenConnectome;
    info.url = infoUrl;
    info.url.setPath(info.url.path().replace(QRegularExpression{R"regex(\/info\/?)regex"}, "/image/jpeg/"));
    const auto jdoc = QJsonDocument::fromJson(json_raw.toUtf8()).object();
    info.experimentname = jdoc["project"]["name"].toString();
    const auto dataset = jdoc["dataset"];
    const auto imagesize0 = dataset["imagesize"]["0"];
    info.boundary = {
        imagesize0[0].toInt(1000),
        imagesize0[1].toInt(1000),
        imagesize0[2].toInt(1000),
    };
    for (auto scaleRef : dataset["voxelres"].toArray()) {
        const auto scale = scaleRef.toArray();
        info.scales.emplace_back(scale[0].toDouble(1), scale[1].toDouble(1), scale[2].toDouble(1));
    }
    info.scale = !info.scales.empty() ? info.scales.front() : decltype(info.scale){1, 1, 1};
    const auto mags = dataset["resolutions"];

    info.magIndex = mags[0].toInt(0);
    info.lowestAvailableMagIndex = info.magIndex;
    info.highestAvailableMagIndex = mags[mags.toArray().size()-1].toInt(0);
    info.type = CubeType::RAW_JPG;

    return {info};
}

Dataset::list_t Dataset::parsePyKnossosConf(const QUrl & configUrl, QString config) {
    Dataset::list_t infos;
    QTextStream stream(&config);
    QString line;
    while (!(line = stream.readLine()).isNull()) {
        const auto & tokenList = line.split(QRegularExpression(" = "));
        const auto & token = tokenList.front();
        if (token.startsWith("[Dataset")) {
            infos.emplace_back();
            infos.back().api = API::PyKnossos;
            continue;
        }
        if (infos.empty()) {// support empty lines in the beginning
            continue;
        }
        const auto value = QString{tokenList.back()}.remove(QChar{'"'});
        auto & info = infos.back();
        if (token == "_BaseName") {
            info.experimentname = value;
        } else if (token == "_BaseURL") {
            QUrl url{line.section(" = ", 1)};// tokelist may split URL in half
            if (!info.url.isEmpty()) {// user info was provided beforehand
                info.url.setHost(url.host());
                info.url.setPath(url.path());
            } else {// read complete url including user info
                info.url = url;
            }
        } else if (token == "_UserName") {
            info.url.setUserName(value);
        } else if (token == "_Password") {
            info.url.setPassword(value);
        } else if (token == "_ServerFormat") {
            info.api = value == "knossos" ? API::Heidelbrain : value == "1" ? API::OpenConnectome : API::PyKnossos;
        } else if (token == "_BaseExt") {
            info.fileextension = value;
            info.type = typeMap.left.at(info.fileextension);
        } else if (token == "_DataScale") {
            const auto tokenList = value.split(",");
            for (int i = 2; i < tokenList.size() - tokenList.back().isEmpty(); i += 3) {
                info.scales.emplace_back(tokenList.at(i-2).toFloat(), tokenList.at(i-1).toFloat(), tokenList.at(i).toFloat());
            }
            info.scale = info.scales.front();
            info.magIndex = 0;
            info.lowestAvailableMagIndex = 0;
            info.highestAvailableMagIndex = tokenList.size() / 3 - 1;
        } else if (token == "_FileType") {
            const auto type = value.toInt();
            if (type == 1) {
                qWarning() << "_FileType = 1 (PNG, rotated) not supported, loading as 2 (PNG)";
            }
            info.type = type == 3 ? CubeType::RAW_JPG : type == 0 ? CubeType::RAW_UNCOMPRESSED : CubeType::RAW_PNG;
        } else if (token == "_Extent") {
            const auto tokenList = value.split(",");
            info.boundary = Coordinate(tokenList.at(0).toFloat(), tokenList.at(1).toFloat(), tokenList.at(2).toFloat());
        } else if (token == "_CubeSize") {
            info.cubeShape = {value.split(",").at(0).toInt(), value.split(",").at(1).toInt(), value.split(",").at(2).toInt()};
            info.gpuCubeShape = info.cubeShape;// possibly ÷2
            info.gpuCubeShape.z = std::max(1, info.gpuCubeShape.z);// 1/2=0 → 1
        } else if (token == "_Color") {
            info.renderSettings.color = QColor{value};
        } else if (token == "_Visible") {
            infos.back().renderSettings.visibleSetExplicitly = true;
            infos.back().allocationEnabled = infos.back().loadingEnabled = info.renderSettings.visible = QVariant{value}.toBool();
        } else if (token == "_Description") {
            info.description = value;
        } else if (!token.isEmpty() && token != "_NumberofCubes" && token != "_Origin") {
            qDebug() << "Skipping unknown parameter" << token;
        }
    }

    for (auto && info : infos) {
        if (info.scales.empty()) {
            return {};
        }
        if (info.url.isEmpty()) {
            info.url = QUrl::fromLocalFile(QFileInfo(configUrl.toLocalFile()).absoluteDir().absolutePath());
        }
        if (&info != &infos.front() && !info.renderSettings.visibleSetExplicitly && !info.isOverlay()) {// disable all non-seg layers expect the first TODO multi layer
            info.allocationEnabled = info.loadingEnabled = info.renderSettings.visible = false;
        }
    }
    return infos;
}

#include <toml.hpp>

extern toml::value toml_parse(const std::vector<unsigned char> & blob, const std::string & filename);

Dataset::list_t Dataset::parseToml(const QUrl & configUrl, QString configData) {
    const auto data = configData.toStdString();
    auto config = toml_parse({std::cbegin(data), std::cend(data)}, configUrl.toString().toStdString());
    Dataset::list_t infos;
    for (const auto & vit : toml::find(config, "Layer").as_array()) {
        Dataset info;
        const auto & value = toml::find_or(vit, "ServerFormat", "");
        info.api = value == "knossos" ? API::Heidelbrain : value == "1" ? API::OpenConnectome : API::PyKnossos;
        info.url = QString::fromStdString(toml::find_or(vit, "URL", std::string{}));
        info.experimentname = QString::fromStdString(toml::find(vit, "Name").as_string());
        const auto & extent = toml::find(vit, "Extent_px").as_array();
        info.boundary = Coordinate(extent.at(0).as_integer(), extent.at(1).as_integer(), extent.at(2).as_integer());
        const auto & cube_shape = toml::find(vit, "CubeShape_px").as_array();
        info.cubeShape = Coordinate(cube_shape.at(0).as_integer(), cube_shape.at(1).as_integer(), cube_shape.at(2).as_integer());
        info.gpuCubeShape = info.cubeShape;// possibly ÷2
        info.gpuCubeShape.z = std::max(1, info.gpuCubeShape.z);// 1/2=0 → 1
        const auto & scales = toml::find(vit, "VoxelSize_nm").as_array();
        for (const auto & scaleit : scales) {
            const auto scale = scaleit.as_array();
            const auto x = (scale.at(0).is_floating()) ? scale.at(0).as_floating() : scale.at(0).as_integer();
            const auto y = (scale.at(1).is_floating()) ? scale.at(1).as_floating() : scale.at(1).as_integer();
            const auto z = (scale.at(2).is_floating()) ? scale.at(2).as_floating() : scale.at(2).as_integer();
            info.scales.emplace_back(x, y, z);
        }
        info.scale = info.scales.front();
        info.magIndex = info.lowestAvailableMagIndex = 0;
        info.highestAvailableMagIndex = scales.size() - 1;
        info.description = QString::fromStdString(toml::find(vit, "Description").as_string());
        info.renderSettings.visibleSetExplicitly = vit.contains("Visible");
        info.allocationEnabled = info.loadingEnabled = info.renderSettings.visible = toml::find_or(vit, "Visible", true);
        info.renderSettings.color = QColor{QString::fromStdString(toml::find_or(vit, "Color", "white"))};

        for (const auto & ext : toml::find(vit, "FileExtension").as_array()) {
            info.fileextension = QString::fromStdString(ext.as_string());
            info.type = typeMap.left.at(info.fileextension);
            infos.emplace_back(info);
        }
    }
    for (auto && info : infos) {
        if (info.scales.empty()) {
            return {};
        }
        if (info.url.isEmpty()) {
            info.url = QUrl::fromLocalFile(QFileInfo(configUrl.toLocalFile()).absoluteDir().absolutePath());
        }
        if (&info != &infos.front() && !info.renderSettings.visibleSetExplicitly && !info.isOverlay()) {// disable all non-seg layers expect the first TODO multi layer
            info.allocationEnabled = info.loadingEnabled = info.renderSettings.visible = false;
        }
    }
    return infos;
}

Dataset::list_t Dataset::parseWebKnossosJson(const QUrl &, const QString & json_raw) {
    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();
    decltype(Dataset::datasets) layers;
    for (const auto & layer : static_cast<const QJsonArray>(jmap["dataSource"]["dataLayers"].toArray())) {
        Dataset info;
        info.api = API::WebKnossos;

        info.url = jmap["dataStore"]["url"].toString() + "/data/datasets/" + jmap["dataSource"]["id"]["team"].toString() + '/' + jmap["dataSource"]["id"]["name"].toString();
        info.experimentname = jmap["name"].toString();

        const auto layerString = layer["name"].toString();
        const auto category = layer["category"].toString();
        const auto download = Network::singleton().refresh(QString("https://demo.webknossos.org/api/userToken/generate"));
        if (download.first) {
            info.token = QJsonDocument::fromJson(download.second.toUtf8())["token"].toString();
        }
        if (category == "color") {
            info.type = CubeType::RAW_UNCOMPRESSED;
        } else {// "segmentation"
            info.type = CubeType::SEGMENTATION_UNCOMPRESSED_16;
        }
        const auto boundary_json = layer["boundingBox"];
        info.boundary = {
            boundary_json["width"].toInt(),
            boundary_json["height"].toInt(),
            boundary_json["depth"].toInt(),
        };

        const auto scale_json = jmap["dataSource"]["scale"];
        info.scale = {
            static_cast<float>(scale_json[0].toDouble()),
            static_cast<float>(scale_json[1].toDouble()),
            static_cast<float>(scale_json[2].toDouble()),
        };

        for (const auto & mag : static_cast<const QJsonArray>(layer["resolutions"].toArray())) {
            info.scales.emplace_back(info.scale.componentMul(floatCoordinate{
                static_cast<float>(mag[0].toDouble()),
                static_cast<float>(mag[1].toDouble()),
                static_cast<float>(mag[2].toDouble())
            }));
        }

        info.magIndex = 0;
        info.lowestAvailableMagIndex = 0;
        info.highestAvailableMagIndex = static_cast<int>(info.scales.size() - 1);

        layers.push_back(info);
        layers.back().url.setPath(info.url.path() + "/layers/" + layerString + "/data");
        layers.back().url.setQuery(info.url.query().append("token=" + info.token));
    }

    return layers;
}

Dataset::list_t Dataset::fromLegacyConf(const QUrl & configUrl, QString config) {
    Dataset info;
    info.api = API::Heidelbrain;
    bool hasPNG{false};

    QTextStream stream(&config);
    QString line;
    boost::optional<std::size_t> magnification;
    while (!(line = stream.readLine()).isNull()) {
        const QStringList tokenList = line.split(QRegularExpression("[ ;]"));
        QString token = tokenList.front();

        if (token == "experiment") {
            info.experimentname = line.split("\"")[1];
        } else if (token == "scale") {
            token = tokenList.at(1);
            if(token == "x") {
                info.scale.x = tokenList.at(2).toFloat();
            } else if(token == "y") {
                info.scale.y = tokenList.at(2).toFloat();
            } else if(token == "z") {
                info.scale.z = tokenList.at(2).toFloat();
            }
        } else if (token == "boundary") {
            token = tokenList.at(1);
            if(token == "x") {
                info.boundary.x = tokenList.at(2).toFloat();
            } else if (token == "y") {
                info.boundary.y = tokenList.at(2).toFloat();
            } else if (token == "z") {
                info.boundary.z = tokenList.at(2).toFloat();
            }
        } else if (token == "magnification") {
            magnification = tokenList.at(1).toULongLong();
        } else if (token == "cube_edge_length") {
            info.cubeShape = {tokenList.at(1).toInt(), tokenList.at(1).toInt(), tokenList.at(1).toInt()};
            info.gpuCubeShape = info.cubeShape;// possibly ÷2
            info.gpuCubeShape.z = std::max(1, info.gpuCubeShape.z);// 1/2=0 → 1
        } else if (token == "ftp_mode") {
            auto urltokenList = tokenList;
            urltokenList.pop_front();
            auto maybeUrl = urltokenList.join("");
            if (QUrl{maybeUrl}.scheme().isEmpty()) {
                maybeUrl.prepend("http://");
            }
            info.url = maybeUrl;
            if (tokenList.size() >= 3 && !tokenList.at(2).isEmpty()) {
                info.url.setPath(tokenList.at(2));
            }
            if (tokenList.size() >= 5) {
                info.url.setUserName(tokenList.at(3));
                info.url.setPassword(tokenList.at(4));
            }
            // discarding ftpFileTimeout parameter
        } else if (token == "compression_ratio") {
            const auto compressionRatio = tokenList.at(1).toInt();
            info.type = compressionRatio == 0 ? Dataset::CubeType::RAW_UNCOMPRESSED
                      : compressionRatio == 1000 ? Dataset::CubeType::RAW_JPG
                      : compressionRatio == 6 ? Dataset::CubeType::RAW_JP2_6
                      : Dataset::CubeType::RAW_J2K;
        } else if (token == "png") {
            hasPNG = true;
        } else {
            qDebug() << "Skipping unknown parameter" << token;
        }
    }

    if (info.url.isEmpty()) {
        //find root of dataset if conf was inside mag folder
        auto configDir = QFileInfo(configUrl.toLocalFile()).absoluteDir();
        if (QRegularExpression("^mag[0-9]+$").match(configDir.dirName()).hasMatch()) {
            configDir.cdUp();//support
        }
        info.url = QUrl::fromLocalFile(configDir.absolutePath());
    }

    if (magnification) {
        //transform boundary and scale of higher mag only datasets
        info.boundary = info.boundary * magnification.get();
        info.scale = info.scale / static_cast<float>(magnification.get());
        info.lowestAvailableMagIndex = info.highestAvailableMagIndex = magnification.get();
    }

    if (info.type != Dataset::CubeType::RAW_UNCOMPRESSED) {
        auto info2 = info;
        info2.type = hasPNG ? Dataset::CubeType::RAW_PNG : Dataset::CubeType::RAW_UNCOMPRESSED;
        info2.allocationEnabled = info2.loadingEnabled = info2.renderSettings.visible = false;
        return {info, info2, info.createCorrespondingOverlayLayer()};
    } else {
        info.type = hasPNG ? Dataset::CubeType::RAW_PNG : Dataset::CubeType::RAW_UNCOMPRESSED;
        return {info, info.createCorrespondingOverlayLayer()};
    }
}

void Dataset::checkMagnifications() {
    //iterate over all possible mags and test their availability
    std::tie(lowestAvailableMagIndex, highestAvailableMagIndex) = Network::singleton().checkOnlineMags(url);
//    magnification = std::min(std::max(magnification, lowestAvailableMag), highestAvailableMag);
    qDebug() << QObject::tr("Lowest Mag: %1, Highest Mag: %2").arg(lowestAvailableMagIndex).arg(highestAvailableMagIndex).toUtf8().constData();
}

Dataset Dataset::createCorrespondingOverlayLayer() {
    Dataset info = *this;
    info.type = (api == API::Heidelbrain || api == API::PyKnossos) ? CubeType::SEGMENTATION_SZ_ZIP : CubeType::SEGMENTATION_UNCOMPRESSED_64;
    return info;
}

QUrl Dataset::knossosCubeUrl(const CoordOfCube cubeCoord) const {
    const auto mag = api == API::PyKnossos ? magIndex + 1 : std::pow(2, magIndex);
    auto pos = QString("/mag%1/x%2/y%3/z%4/")
            .arg(mag)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));
    auto filename = QString(("%1_mag%2_x%3_y%4_z%5%6"))//2012-03-07_AreaX14_mag1_x0000_y0000_z0000.j2k
            .arg(experimentname.section(QString("_mag"), 0, 0))
            .arg(mag)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'))
            .arg(typeMap.right.at(type));
    auto base = url;
    base.setPath(url.path() + pos + filename);
    return base;
}

QUrl Dataset::openConnectomeCubeUrl(CoordOfCube coord) const {
    auto path = url.path();

    path += (!path.endsWith('/') ? "/" : "") + QString::number(magIndex);// >= 0
    coord.x *= cubeShape.x;
    coord.y *= cubeShape.y;
    coord.z += 1;//offset
    path += "/" + QString("%1,%2").arg(coord.x).arg(coord.x + cubeShape.x);
    path += "/" + QString("%1,%2").arg(coord.y).arg(coord.y + cubeShape.y);
    path += "/" + QString("%1,%2").arg(coord.z).arg(coord.z + cubeShape.z);

    auto base = url;
    base.setPath(path + "/");
    //(string: server_name)/ocp/ca/(string: token_name)/(string: channel_name)/jpeg/(int: resolution)/(int: min_x),(int: max_x)/(int: min_y),(int: max_y)/(int: min_z),(int: max_z)/
    //(string: server_name)/nd/sd/(string: token_name)/(string: channel_name)/jpeg/(int: resolution)/(int: min_x),(int: max_x)/(int: min_y),(int: max_y)/(int: min_z),(int: max_z)/
    return base;
}

QNetworkRequest Dataset::apiSwitch(const CoordOfCube cubeCoord) const {
    switch (api) {
    case API::GoogleBrainmaps: {
        QNetworkRequest request{url.toString() + "/subvolume:binary"};
        request.setRawHeader("Authorization", (QString("Bearer ") + token).toUtf8());
        return request;
    }
    case API::Heidelbrain:
    case API::PyKnossos: {
        return QNetworkRequest{knossosCubeUrl(cubeCoord)};
    }
    case API::OpenConnectome:
        return QNetworkRequest{openConnectomeCubeUrl(cubeCoord)};
    case API::WebKnossos:
        return QNetworkRequest{url};
    }
    throw std::runtime_error("unknown value for Dataset::API");
}

bool Dataset::isOverlay() const {
    return type == CubeType::SEGMENTATION_UNCOMPRESSED_16
            || type == CubeType::SEGMENTATION_UNCOMPRESSED_64
            || type == CubeType::SEGMENTATION_SZ_ZIP
            || type == CubeType::SNAPPY;
}

std::size_t Dataset::toMagIndex(const int mag) const {
    return api == API::Heidelbrain ? std::log2(mag) : mag - 1;
}

int Dataset::toMag(const std::size_t magIndex) const {
    return api == API::Heidelbrain ? std::pow(2, magIndex) : magIndex + 1;
}
