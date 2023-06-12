/*
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  This file is a part of KNOSSOS.
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

#pragma once

#include "coordinate.h"

#include <QOpenGLTexture>

#include <QColor>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>

#include <boost/container/small_vector.hpp>

struct Dataset {
    using list_t = boost::container::small_vector<Dataset, 2>;
    enum class API {
        Heidelbrain, WebKnossos, GoogleBrainmaps, PyKnossos, OpenConnectome
    };
    enum class CubeType {
        RAW_UNCOMPRESSED, RAW_JPG, RAW_J2K, RAW_JP2_6, RAW_PNG, SEGMENTATION_UNCOMPRESSED_16, SEGMENTATION_UNCOMPRESSED_64, SEGMENTATION_SZ_ZIP, SNAPPY
    };
    QString compressionString() const;
    QString apiString() const;

    static bool isGoogleBrainmaps(const QUrl & url);
    static bool isHeidelbrain(const QUrl & url);
    static bool isNeuroDataStore(const QUrl & url);
    static bool isPyKnossos(const QUrl & url);
    static bool isToml(const QUrl & url);
    static bool isWebKnossos(const QUrl & url);

    static list_t parse(const QUrl & url, const QString &data, bool add_snappy);
    static list_t parseGoogleJson(const QUrl & infoUrl, const QString & json_raw);
    static list_t parseNeuroDataStoreJson(const QUrl & infoUrl, const QString & json_raw);
    static list_t parsePyKnossosConf(const QUrl & configUrl, QString config);
    static list_t parseToml(const QUrl & configUrl, QString config);
    static list_t parseWebKnossosJson(const QUrl &infoUrl, const QString & json_raw);
    static list_t fromLegacyConf(const QUrl & url, QString config);
    void checkMagnifications();
    Dataset createCorrespondingOverlayLayer();

    QNetworkRequest apiSwitch(const CoordOfCube cubeCoord) const;
    QUrl knossosCubeUrl(const CoordOfCube cubeCoord) const;
    QUrl openConnectomeCubeUrl(CoordOfCube coord) const;

    bool isOverlay() const;

    CoordOfCube global2cube(const Coordinate & globalCoord) const {
        return globalCoord.cube(cubeShape, scaleFactor);
    }
    Coordinate cube2global(const CoordOfCube & cubeCoord) const {
        return cubeCoord.cube2Global(cubeShape, scaleFactor);
    }

    API api{API::Heidelbrain};
    CubeType type{CubeType::RAW_UNCOMPRESSED};
    // Edge length of the current data set in data pixels.
    Coordinate boundary{1000, 1000, 1000};
    // pixel-to-nanometer scale
    floatCoordinate scale{1.f, 1.f, 1.f};
    boost::container::small_vector<floatCoordinate, 4> scales;
    // stores the currently active magnification;
    // it is set by magnification = 2^MAGx
    // Dataset::current().magnification should only be used by the viewer,
    // but its value is copied over to loaderMagnification.
    // This is locked for thread safety.
    // do not change to uint, it causes bugs in the display of higher mag datasets
    std::size_t magIndex{0};
    int magnification{1};
    int lowestAvailableMag{1};
    int highestAvailableMag{1};
    floatCoordinate scaleFactor{1,1,1};
    // The edge length of a datacube is 2^N, which makes the size of a
    // datacube in bytes 2^3N which has to be <= 2^32 - 1 (unsigned int).
    // So N cannot be larger than 10.
    // Edge length of one cube in pixels: 2^N
    Coordinate cubeShape{128, 128, 128};
    Coordinate gpuCubeShape{128, 128, 128};
    QString description;
    // Current dataset identifier string
    QString experimentname;
    QString fileextension;
    QUrl url;
    QString token;
    bool allocationEnabled{true};
    bool loadingEnabled{true};

    struct LayerRenderSettings {
        bool visibleSetExplicitly{false};
        bool visible{true};
        double opacity{1.0};
        double rangeDelta{1.};
        double bias{0.};
        bool combineSlicesEnabled{false};
        bool combineSlicesXyOnly{true};
        enum : int {
            min, max
        } combineSlicesType{};
        int combineSlices{4};
        QOpenGLTexture::Filter textureFilter{QOpenGLTexture::Nearest};
        QColor color{Qt::white};
    } renderSettings;

    static Dataset::list_t datasets;
    static auto & current() {
        if (!datasets.empty()) {
            return datasets.front();
        } else {
            static Dataset dummy;
            dummy.scales = {dummy.scale};
            return dummy;
        }
    }
};
