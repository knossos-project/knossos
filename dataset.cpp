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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "dataset.h"

#include "network.h"
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

Dataset Dataset::dummyDataset() {
    Dataset info;
    info.api = API::Heidelbrain;
    info.boundary = {1000, 1000, 1000};
    info.scale = {1.f, 1.f, 1.f};
    info.lowestAvailableMag = 1;
    info.magnification = 1;
    info.highestAvailableMag = 1;
    info.cubeEdgeLength = 128;
    info.overlay = false;
    return info;
}

Dataset Dataset::parseGoogleJson(const QString & json_raw) {
    Dataset info;
    info.api = API::GoogleBrainmaps;
    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();

    const auto boundary_json = jmap["geometry"].toArray()[0].toObject()["volumeSize"].toObject();
    info.boundary = {
        boundary_json["x"].toString().toInt(),
        boundary_json["y"].toString().toInt(),
        boundary_json["z"].toString().toInt(),
    };

    const auto scale_json = jmap["geometry"].toArray()[0].toObject()["pixelSize"].toObject();
    info.scale = {
        static_cast<float>(scale_json["x"].toDouble()),
        static_cast<float>(scale_json["y"].toDouble()),
        static_cast<float>(scale_json["z"].toDouble()),
    };

    info.lowestAvailableMag = 1;
    info.magnification = info.lowestAvailableMag;
    info.highestAvailableMag = std::pow(2,(jmap["geometry"].toArray().size()-1)); //highest google mag
    info.compressionRatio = 1000;//jpeg
    info.overlay = false;

    return info;
}

Dataset Dataset::parseOpenConnectomeJson(const QUrl & infoUrl, const QString & json_raw) {
    Dataset info;
    info.api = API::OpenConnectome;
    info.url = infoUrl;
    info.url.setPath(info.url.path().replace("/info", "/image/jpeg/"));
    const auto dataset = QJsonDocument::fromJson(json_raw.toUtf8()).object()["dataset"].toObject();
    const auto imagesize0 = dataset["imagesize"].toObject()["0"].toArray();
    info.boundary = {
        imagesize0[0].toInt(),
        imagesize0[1].toInt(),
        imagesize0[2].toInt(),
    };
    const auto voxelres0 = dataset["voxelres"].toObject()["0"].toArray();
    info.scale = {
        static_cast<float>(voxelres0[0].toDouble()),
        static_cast<float>(voxelres0[1].toDouble()),
        static_cast<float>(voxelres0[2].toDouble()),
    };
    const auto mags = dataset["resolutions"].toArray();

    info.lowestAvailableMag = std::pow(2, mags[0].toInt());
    info.magnification = info.lowestAvailableMag;
    info.highestAvailableMag = std::pow(2, mags[mags.size()-1].toInt());
    info.compressionRatio = 1000;//jpeg
    info.overlay = false;

    return info;
}

Dataset Dataset::parseWebKnossosJson(const QString & json_raw) {
    Dataset info;
    info.api = API::WebKnossos;
    const auto jmap = QJsonDocument::fromJson(json_raw.toUtf8()).object();

    const auto boundary_json = jmap["dataSource"].toObject()["dataLayers"].toArray()[1].toObject()["sections"].toArray()[0].toObject()["bboxBig"].toObject(); //use bboxBig from color because its bigger :X
    info.boundary = {
        boundary_json["width"].toInt(),
        boundary_json["height"].toInt(),
        boundary_json["depth"].toInt(),
    };

    const auto scale_json = jmap["dataSource"].toObject()["scale"].toArray();
    info.scale = {
        static_cast<float>(scale_json[0].toDouble()),
        static_cast<float>(scale_json[1].toDouble()),
        static_cast<float>(scale_json[2].toDouble()),
    };

    const auto mags = jmap["dataSource"].toObject()["dataLayers"].toArray()[0].toObject()["sections"].toArray()[0].toObject()["resolutions"].toArray();

    info.lowestAvailableMag = mags[0].toInt();
    info.magnification = info.lowestAvailableMag;
    info.highestAvailableMag = mags[mags.size()-1].toInt();
    info.compressionRatio = 0;//raw
    info.overlay = false;

    return info;
}

Dataset Dataset::fromLegacyConf(const QUrl & configUrl, QString config) {
    Dataset info;
    info.api = API::Heidelbrain;

    QTextStream stream(&config);
    QString line;
    while (!(line = stream.readLine()).isNull()) {
        const QStringList tokenList = line.split(QRegularExpression("[ ;]"));
        QString token = tokenList.front();

        if (token == "experiment") {
            token = tokenList.at(2);
            info.experimentname = token.remove('\"');
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
            info.magnification = tokenList.at(1).toInt();
        } else if (token == "cube_edge_length") {
            info.cubeEdgeLength = tokenList.at(1).toInt();
        } else if (token == "ftp_mode") {
            info.remote = true;
            info.url.setScheme("http");
            info.url.setUserName(tokenList.at(3));
            info.url.setPassword(tokenList.at(4));
            info.url.setHost(tokenList.at(1));
            info.url.setPath(tokenList.at(2));
            //discarding ftpFileTimeout parameter
        } else if (token == "compression_ratio") {
            info.compressionRatio = tokenList.at(1).toInt();
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

    //transform boundary and scale of higher mag only datasets
    info.boundary = info.boundary * info.magnification;
    info.scale = info.scale / static_cast<float>(info.magnification);
    info.lowestAvailableMag = info.highestAvailableMag = info.magnification;

    return info;
}

void Dataset::checkMagnifications() {
    //iterate over all possible mags and test their availability
    std::tie(lowestAvailableMag, highestAvailableMag) = Network::singleton().checkOnlineMags(url);
    qDebug() << QObject::tr("Lowest Mag: %1, Highest Mag: %2").arg(lowestAvailableMag).arg(highestAvailableMag);
}

void Dataset::applyToState() const {
    state->magnification = magnification;
    state->lowestAvailableMag = lowestAvailableMag;
    state->highestAvailableMag = highestAvailableMag;
    //boundary and scale were already converted
    state->boundary = boundary;
    state->scale = scale;
    state->name = experimentname;
    state->cubeEdgeLength = cubeEdgeLength;
    state->compressionRatio = compressionRatio;
    state->overlay = overlay;

    Skeletonizer::singleton().skeletonState.volBoundary = SkeletonState{}.volBoundary;
}

QUrl knossosCubeUrl(QUrl base, QString && experimentName, const Coordinate & coord, const int cubeEdgeLength, const int magnification, const Dataset::CubeType type) {
    const auto cubeCoord = coord.cube(cubeEdgeLength, magnification);
    auto pos = QString("/mag%1/x%2/y%3/z%4/")
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));
    auto filename = QString(("%1_mag%2_x%3_y%4_z%5%6"))//2012-03-07_AreaX14_mag1_x0000_y0000_z0000.j2k
            .arg(experimentName.section(QString("_mag"), 0, 0))
            .arg(magnification)
            .arg(cubeCoord.x, 4, 10, QChar('0'))
            .arg(cubeCoord.y, 4, 10, QChar('0'))
            .arg(cubeCoord.z, 4, 10, QChar('0'));

    if (type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        filename = filename.arg(".raw");
    } else if (type == Dataset::CubeType::RAW_JPG) {
        filename = filename.arg(".jpg");
    } else if (type == Dataset::CubeType::RAW_J2K) {
        filename = filename.arg(".j2k");
    } else if (type == Dataset::CubeType::RAW_JP2_6) {
        filename = filename.arg(".6.jp2");
    } else if (type == Dataset::CubeType::SEGMENTATION_SZ_ZIP) {
        filename = filename.arg(".seg.sz.zip");
    } else if (type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED) {
        filename = filename.arg(".seg");
    }

    base.setPath(base.path() + pos + filename);

    return base;
}

QUrl googleCubeUrl(QUrl base, Coordinate coord, const int scale, const int cubeEdgeLength, const Dataset::CubeType type) {
    auto query = QUrlQuery(base);
    auto path = base.path() + "/binary/subvolume";

    if (type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        path += "/format=raw";
    } else if (type == Dataset::CubeType::RAW_JPG) {
        path += "/format=singleimage";
    }

    path += "/scale=" + QString::number(scale);// >= 0
    path += "/size=" + QString("%1,%1,%1").arg(cubeEdgeLength);// <= 128³
    path += "/corner=" + QString("%1,%2,%3").arg(coord.x).arg(coord.y).arg(coord.z);

    query.addQueryItem("alt", "media");

    base.setPath(path);
    base.setQuery(query);
    //<volume_id>/binary/subvolume/corner=5376,5504,2944/size=64,64,64/scale=0/format=singleimage?access_token=<oauth2_token>
    return base;
}

QUrl openConnectomeCubeUrl(QUrl base, Coordinate coord, const int scale, const int cubeEdgeLength) {
    auto query = QUrlQuery(base);
    auto path = base.path();

    path += "/" + QString::number(scale);// >= 0
    coord.x /= std::pow(2, scale);
    coord.y /= std::pow(2, scale);
    coord.z += 1;//offset
    path += "/" + QString("%1,%2").arg(coord.x).arg(coord.x + cubeEdgeLength);
    path += "/" + QString("%1,%2").arg(coord.y).arg(coord.y + cubeEdgeLength);
    path += "/" + QString("%1,%2").arg(coord.z).arg(coord.z + cubeEdgeLength);

    base.setPath(path + "/");
    base.setQuery(query);
    //(string: server_name)/ocp/ca/(string: token_name)/(string: channel_name)/jpeg/(int: resolution)/(int: min_x),(int: max_x)/(int: min_y),(int: max_y)/(int: min_z),(int: max_z)/
    return base;
}

QUrl webKnossosCubeUrl(QUrl base, Coordinate coord, const int unknownScale, const int cubeEdgeLength, const Dataset::CubeType type) {
    auto query = QUrlQuery(base);
    query.addQueryItem("cubeSize", QString::number(cubeEdgeLength));

    QString layer;
    if (type == Dataset::CubeType::RAW_UNCOMPRESSED) {
        layer = "color";
    } else if (type == Dataset::CubeType::SEGMENTATION_UNCOMPRESSED) {
        layer = "volume";
    }

    auto path = base.path() + "/layers/" + layer + "/mag%1/x%2/y%3/z%4/bucket.raw";//mag >= 1
    path = path.arg(unknownScale).arg(coord.x / state->cubeEdgeLength).arg(coord.y / state->cubeEdgeLength).arg(coord.z / state->cubeEdgeLength);
    base.setPath(path);
    base.setQuery(query);

    return base;
}

QUrl Dataset::apiSwitch(const API api, const QUrl & baseUrl, const Coordinate globalCoord, const int scale, const int cubeedgelength, const CubeType type) {
    switch (api) {
    case API::GoogleBrainmaps:
        return googleCubeUrl(baseUrl, globalCoord, scale, cubeedgelength, type);
    case API::Heidelbrain:
        return knossosCubeUrl(baseUrl, QString(state->name), globalCoord, cubeedgelength, std::pow(2, scale), type);
    case API::OpenConnectome:
        return openConnectomeCubeUrl(baseUrl, globalCoord, scale, cubeedgelength);
    case API::WebKnossos:
        return webKnossosCubeUrl(baseUrl, globalCoord, scale + 1, cubeedgelength, type);
    }
    throw std::runtime_error("unknown value for Dataset::API");
}

bool Dataset::isOverlay(const CubeType type) {
    switch (type) {
    case CubeType::RAW_UNCOMPRESSED:
    case CubeType::RAW_JPG:
    case CubeType::RAW_J2K:
    case CubeType::RAW_JP2_6:
        return false;
    case CubeType::SEGMENTATION_UNCOMPRESSED:
    case CubeType::SEGMENTATION_SZ_ZIP:
        return true;
    };
    throw std::runtime_error("unknown value for Dataset::CubeType");
}
