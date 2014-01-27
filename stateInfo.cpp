#include "knossos-global.h"

stateInfo::stateInfo() {

}

uint stateInfo::getSvnRevision() {
    return svnRevision;
}

float stateInfo::getAlpha() {
    return alpha;
}

float stateInfo::getBeta() {
    return beta;
}

bool stateInfo::isOverlay() {
    return overlay;
}

bool stateInfo::isLoaderBusy() {
    return loaderBusy;
}

bool stateInfo::isLoaderDummy() {
    return loaderDummy;
}

bool stateInfo::isQuitSignal() {
    return quitSignal;
}

bool stateInfo::isClientSignal() {
    return clientSignal;
}

bool stateInfo::isRemoteSignal() {
    return remoteSignal;
}

bool stateInfo::isBoergens() {
    return boergens;
}

char *stateInfo::getPath() {
    return path;
}

char *stateInfo::getLoaderPath() {
    return loaderPath;
}

char **stateInfo::getMagNames() {
    return (char **)magNames;
}

char **stateInfo::getMagPaths() {
    return (char **)magPaths;
}

char *stateInfo::getDatasetBaseExpName() {
    return datasetBaseExpName;
}

int stateInfo::getMagnification() {
    return magnification;
}

uint stateInfo::getCompressionRatio() {
    return compressionRatio;
}

uint stateInfo::getHighestAvailableMag() {
    return highestAvailableMag;
}

uint stateInfo::getLowestAvailableMag() {
    return lowestAvailableMag;
}

uint stateInfo::getLoaderMagnification() {
    return loaderMagnification;
}

uint stateInfo::getCubeBytes() {
    return cubeBytes;
}

int stateInfo::getCubeEdgeLength() {
    return cubeEdgeLength;
}

int stateInfo::getCubeSliceArea() {
    return cubeSliceArea;
}

int stateInfo::getM() {
    return M;
}

uint stateInfo::getCubeSetElements() {
    return cubeSetElements;
}

uint stateInfo::getCubeSetBytes() {
    return cubeSetBytes;
}

Coordinate stateInfo::getBoundary() {
    return boundary;
}

