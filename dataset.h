/*
 *  (C) Copyright 2007-2016
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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef DATASET_H
#define DATASET_H

#include "coordinate.h"

#include <QString>
#include <QUrl>

struct Dataset {
    enum class API {
        Heidelbrain, WebKnossos, GoogleBrainmaps, OpenConnectome
    };
    enum class CubeType {
        RAW_UNCOMPRESSED, RAW_JPG, RAW_J2K, RAW_JP2_6, SEGMENTATION_UNCOMPRESSED, SEGMENTATION_SZ_ZIP
    };
    QString compressionString() const;

    static bool isNeuroDataStore(const QUrl & url);

    static Dataset parseGoogleJson(const QString & json_raw);
    static Dataset parseNeuroDataStoreJson(const QUrl & infoUrl, const QString & json_raw);
    static Dataset parseWebKnossosJson(const QString & json_raw);
    static Dataset fromLegacyConf(const QUrl & url, QString config);
    void checkMagnifications();

    static QUrl apiSwitch(const API api, const QUrl & baseUrl, const Coordinate globalCoord, const int scale, const int cubeedgelength, const CubeType type);
    static bool isOverlay(const CubeType type);

    API api{API::Heidelbrain};
    CubeType type{CubeType::RAW_UNCOMPRESSED};
    // Edge length of the current data set in data pixels.
    Coordinate boundary{1000, 1000, 1000};
    // pixel-to-nanometer scale
    floatCoordinate scale{1.f, 1.f, 1.f};
    // stores the currently active magnification;
    // it is set by magnification = 2^MAGx
    // Dataset::current.magnification should only be used by the viewer,
    // but its value is copied over to loaderMagnification.
    // This is locked for thread safety.
    // do not change to uint, it causes bugs in the display of higher mag datasets
    int magnification{1};
    int lowestAvailableMag{1};
    int highestAvailableMag{1};
    // The edge length of a datacube is 2^N, which makes the size of a
    // datacube in bytes 2^3N which has to be <= 2^32 - 1 (unsigned int).
    // So N cannot be larger than 10.
    // Edge length of one cube in pixels: 2^N
    int cubeEdgeLength{128};
    bool remote{false};
    bool overlay{false};
    // Current dataset identifier string
    QString experimentname;
    QUrl url;

    static std::vector<Dataset> datasets;
    static Dataset & current;
};

#endif//DATASET_H
