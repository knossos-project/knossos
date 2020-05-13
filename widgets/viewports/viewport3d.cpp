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

#include "viewport3d.h"

#include "dataset.h"
#include "profiler.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"

Viewport3D::Viewport3D(QWidget *parent, ViewportType viewportType) : ViewportBase(parent, viewportType) {
    wiggleButton.setCheckable(true);
    wiggleButton.setToolTip("Wiggle stereoscopy (Hold W)");

    for (auto * button : {&xyButton, &xzButton, &zyButton}) {
        button->setMinimumSize(30, 20);
    }
    wiggleButton.setMinimumSize(35, 20);
    r90Button.setMinimumSize(35, 20);
    r180Button.setMinimumSize(40, 20);
    resetButton.setMinimumSize(45, 20);
    resetButton.setToolTip("Ctrl+0");
    resetAction.setShortcut(QKeySequence("Ctrl+0"));
    resetAction.setShortcutContext(Qt::WidgetShortcut);
    QObject::connect(&resetAction, &QAction::triggered, &resetButton, &QPushButton::clicked);

    menuButton.menu()->insertAction(menuButton.menu()->actions().back(), &showVolumeAction);
    showVolumeAction.setCheckable(true);
    QObject::connect(&showVolumeAction, &QAction::triggered, &Segmentation::singleton(), &Segmentation::toggleVolumeRender);
    QObject::connect(&Segmentation::singleton(), &Segmentation::volumeRenderToggled, &showVolumeAction, &QAction::setChecked);

    addAction(&resetAction);

    for (auto * button : {&resetButton, &r180Button, &r90Button, &zyButton, &xzButton, &xyButton, &wiggleButton}) {
        button->setMaximumSize(button->minimumSize());
        button->setCursor(Qt::ArrowCursor);
        vpHeadLayout.insertWidget(0, button);
    }

    connect(&xyButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_XY_VIEW;
        }
    });
    connect(&xzButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_XZ_VIEW;
        }
    });
    connect(&zyButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_ZY_VIEW;
        }
    });
    connect(&r90Button, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_R90;
        }
    });
    connect(&r180Button, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_R180;
        }
    });
    QObject::connect(&wiggleButton, &QPushButton::clicked, [this](bool checked){
        if (checked) {
            wiggletimer.start();
        } else {
            resetWiggle();
        }
    });
    connect(&resetButton, &QPushButton::clicked, []() {
        if(state->skeletonState->rotationcounter == 0) {
            state->skeletonState->definedSkeletonVpView = SKELVP_RESET;
        }
    });

    wiggletimer.setInterval(30);
    QObject::connect(&wiggletimer, &QTimer::timeout, [this](){
        const auto inc = wiggleDirection ? 1 : -1;
        state->skeletonState->rotdx += inc;
        state->skeletonState->rotdy += inc;
        wiggle += inc;
        if (wiggle == -2 || wiggle == 2) {
            wiggleDirection = !wiggleDirection;
        }
    });
}

Viewport3D::~Viewport3D() {
    makeCurrent();
    if (Segmentation::singleton().volume_tex_id != 0) {
        glDeleteTextures(1, &Segmentation::singleton().volume_tex_id);
    }
    Segmentation::singleton().volume_tex_id = 0;
}

void Viewport3D::paintGL() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderViewport();
    renderViewportFrontFace();
}

void Viewport3D::refocus(const boost::optional<Coordinate> position) {
    const auto pos = position ? position.get() : state->viewerState->currentPosition;
    const auto scaledPos = Dataset::current().scales[0].componentMul(pos);
    translateX = scaledPos.x;
    translateY = scaledPos.y;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(QMatrix4x4().data());
    glTranslatef(scaledPos.x, scaledPos.y, scaledPos.z);
    std::array<float, 16> rotationState;
    rotation.copyDataTo(rotationState.data()); // transforms to row-major matrix
    glMultMatrixf(rotationState.data());
    glTranslatef(-scaledPos.x, -scaledPos.y, -scaledPos.z);
    glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);
}

void Viewport3D::updateVolumeTexture() {
    if (!Segmentation::singleton().enabled) {
        return;
    }
    auto& seg = Segmentation::singleton();
    int texLen = seg.volume_tex_len;
    if(seg.volume_tex_id == 0) {
        glGenTextures(1, &seg.volume_tex_id);
        glBindTexture(GL_TEXTURE_3D, seg.volume_tex_id);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texLen, texLen, texLen, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    static Profiler tex_gen_profiler;
    static Profiler dcfetch_profiler;
    static Profiler colorfetch_profiler;
    static Profiler occlusion_profiler;
    static Profiler tex_transfer_profiler;

    tex_gen_profiler.start(); // ----------------------------------------------------------- profiling
    const auto currentPosDc = state->viewerState->currentPosition.cube(Dataset::current().cubeEdgeLength, Dataset::current().scaleFactor);
    int cubeLen = Dataset::current().cubeEdgeLength;
    int M = state->M;
    int M_radius = (M - 1) / 2;
    GLubyte* colcube = new GLubyte[4*texLen*texLen*texLen];
    std::tuple<uint64_t, Segmentation::color_t> lastIdColor;

    state->protectCube2Pointer.lock();

    dcfetch_profiler.start(); // ----------------------------------------------------------- profiling
    uint64_t** rawcubes = new uint64_t*[M*M*M];
    for(int z = 0; z < M; ++z)
    for(int y = 0; y < M; ++y)
    for(int x = 0; x < M; ++x) {
        auto cubeIndex = z*M*M + y*M + x;
        const CoordOfCube cubeCoordRelative{x - M_radius, y - M_radius, z - M_radius};
        rawcubes[cubeIndex] = reinterpret_cast<uint64_t*>(cubeQuery(state->cube2Pointer
            , Segmentation::singleton().layerId, Dataset::current().magIndex, currentPosDc + cubeCoordRelative));
    }
    dcfetch_profiler.end(); // ----------------------------------------------------------- profiling

    colorfetch_profiler.start(); // ----------------------------------------------------------- profiling

    for(int z = 0; z < texLen; ++z)
    for(int y = 0; y < texLen; ++y)
    for(int x = 0; x < texLen; ++x) {
        Coordinate DcCoord{(x * M)/cubeLen, (y * M)/cubeLen, (z * M)/cubeLen};
        auto cubeIndex = DcCoord.z*M*M + DcCoord.y*M + DcCoord.x;
        auto& rawcube = rawcubes[cubeIndex];

        if(rawcube != nullptr) {
            auto indexInDc  = ((z * M)%cubeLen)*cubeLen*cubeLen + ((y * M)%cubeLen)*cubeLen + (x * M)%cubeLen;
            auto indexInTex = z*texLen*texLen + y*texLen + x;
            auto subobjectId = rawcube[indexInDc];
            if(subobjectId == std::get<0>(lastIdColor)) {
                auto idColor = std::get<1>(lastIdColor);
                colcube[4*indexInTex+0] = std::get<0>(idColor);
                colcube[4*indexInTex+1] = std::get<1>(idColor);
                colcube[4*indexInTex+2] = std::get<2>(idColor);
                colcube[4*indexInTex+3] = std::get<3>(idColor);
            } else if (seg.isSubObjectIdSelected(subobjectId)) {
                auto idColor = seg.colorObjectFromSubobjectId(subobjectId);
                std::get<3>(idColor) = 255; // ignore color alpha
                colcube[4*indexInTex+0] = std::get<0>(idColor);
                colcube[4*indexInTex+1] = std::get<1>(idColor);
                colcube[4*indexInTex+2] = std::get<2>(idColor);
                colcube[4*indexInTex+3] = std::get<3>(idColor);
                lastIdColor = std::make_tuple(subobjectId, idColor);
            } else {
                colcube[4*indexInTex+0] = 0;
                colcube[4*indexInTex+1] = 0;
                colcube[4*indexInTex+2] = 0;
                colcube[4*indexInTex+3] = 0;
            }
        } else {
            auto indexInTex = z*texLen*texLen + y*texLen + x;
            colcube[4*indexInTex+0] = 0;
            colcube[4*indexInTex+1] = 0;
            colcube[4*indexInTex+2] = 0;
            colcube[4*indexInTex+3] = 0;
        }
    }

    delete[] rawcubes;

    state->protectCube2Pointer.unlock();

    colorfetch_profiler.end(); // ----------------------------------------------------------- profiling

    occlusion_profiler.start(); // ----------------------------------------------------------- profiling
    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int xi = -1; xi <= 1; ++xi) {
                auto othrIndexInTex = (z)*texLen*texLen + (y)*texLen + x+xi;
                if((xi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }

    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int yi = -1; yi <= 1; ++yi) {
                auto othrIndexInTex = (z)*texLen*texLen + (y+yi)*texLen + x;
                if((yi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }

    for(int z = 1; z < texLen - 1; ++z)
    for(int y = 1; y < texLen - 1; ++y)
    for(int x = 1; x < texLen - 1; ++x) {
        auto indexInTex = (z)*texLen*texLen + (y)*texLen + x;
        if(colcube[4*indexInTex+3] != 0) {
            for(int zi = -1; zi <= 1; ++zi) {
                auto othrIndexInTex = (z+zi)*texLen*texLen + (y)*texLen + x;
                if((zi != 0) && colcube[4*othrIndexInTex+3] != 0) {
                    colcube[4*indexInTex+0] *= 0.95f;
                    colcube[4*indexInTex+1] *= 0.95f;
                    colcube[4*indexInTex+2] *= 0.95f;
                }
            }
        }
    }
    occlusion_profiler.end(); // ----------------------------------------------------------- profiling

    tex_transfer_profiler.start(); // ----------------------------------------------------------- profiling
    glBindTexture(GL_TEXTURE_3D, seg.volume_tex_id);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texLen, texLen, texLen, 0, GL_RGBA, GL_UNSIGNED_BYTE, colcube);
    delete[] colcube;
    tex_transfer_profiler.end(); // ----------------------------------------------------------- profiling

    tex_gen_profiler.end(); // ----------------------------------------------------------- profiling

    // --------------------- display some profiling information ------------------------
    // qDebug() << "tex gen avg time: " << tex_gen_profiler.average_time()*1000 << "ms";
    // qDebug() << "    dc fetch    : " << dcfetch_profiler.average_time()*1000 << "ms";
    // qDebug() << "    color fetch : " << colorfetch_profiler.average_time()*1000 << "ms";
    // qDebug() << "    occlusion   : " << occlusion_profiler.average_time()*1000 << "ms";
    // qDebug() << "    tex transfer: " << tex_transfer_profiler.average_time()*1000 << "ms";
    // qDebug() << "---------------------------------------------";
}

void Viewport3D::showHideButtons(bool isShow) {
    ViewportBase::showHideButtons(isShow);
    xyButton.setVisible(isShow);
    xzButton.setVisible(isShow);
    zyButton.setVisible(isShow);
    r90Button.setVisible(isShow);
    r180Button.setVisible(isShow);
    wiggleButton.setVisible(isShow);
    resetButton.setVisible(isShow);
}

float Viewport3D::zoomStep() const {
    return std::pow(2, 0.25);
}

void Viewport3D::zoom(const float step) {
    auto & zoomLvl = zoomFactor;
    zoomLvl = std::min(std::max(zoomLvl * step, SKELZOOMMIN), SKELZOOMMAX);
    emit updateZoomWidget();
}
