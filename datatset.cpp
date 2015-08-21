#include "datatset.h"

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

Dataset Dataset::dummyDataset() {
    Dataset info;
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

Dataset Dataset::parseWebKnossosJson(const QString & json_raw) {
    Dataset info;
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
    //iterate over all possible mags and test their availability (if directory exists)
    for (int currMag = 1; currMag <= NUM_MAG_DATASETS; currMag *= 2) {
        bool currMagExists = false;
        QUrl magUrl = url;
        magUrl.setPath(QString("%1/mag%2/").arg(url.path()).arg(currMag));
        if (remote) {
            currMagExists = Network::singleton().refresh(magUrl).first;
        } else {
            currMagExists = QDir(magUrl.toLocalFile()).exists();
        }
        if (currMagExists) {
            lowestAvailableMag = std::min(lowestAvailableMag, currMag);
            highestAvailableMag = std::max(highestAvailableMag, currMag);
        }
    }
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

    // update the volume boundary
    if ((state->boundary.x >= state->boundary.y) && (state->boundary.x >= state->boundary.z)) {
        Skeletonizer::singleton().skeletonState.volBoundary = state->boundary.x * 2;
    }
    if ((state->boundary.y >= state->boundary.x) && (state->boundary.y >= state->boundary.z)) {
        Skeletonizer::singleton().skeletonState.volBoundary = state->boundary.y * 2;
    }
    if ((state->boundary.z >= state->boundary.x) && (state->boundary.z >= state->boundary.y)) {
        Skeletonizer::singleton().skeletonState.volBoundary = state->boundary.z * 2;
    }
}
