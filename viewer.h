/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2012
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

static struct vpList *vpListNew();
static int32_t vpListAddElement(struct vpList *vpList, struct viewPort *viewPort, struct vpBacklog *backlog);
static struct vpList *vpListGenerate(struct viewerState *viewerState);
static int32_t vpListDelElement(struct vpList *list, struct vpListElement *element);
static int32_t vpListDel(struct vpList *list);
static struct vpBacklog *backlogNew();
static int32_t backlogDelElement(struct vpBacklog *backlog, struct vpBacklogElement *element);
static int32_t backlogAddElement(struct vpBacklog *backlog, Coordinate datacube, uint32_t dcOffset, Byte *slice, uint32_t x_px, uint32_t y_px, uint32_t cubeType);
static int32_t backlogDel(struct vpBacklog *backlog);
static int32_t vpHandleBacklog(struct vpListElement *currentVp, struct viewerState *viewerState);
static uint32_t dcSliceExtract(Byte *datacube,
                               Byte *slice,
                               size_t dcOffset,
                               struct viewPort *viewPort);
static uint32_t sliceExtract_standard(Byte *datacube,
                                      Byte *slice,
                                      struct viewPort *viewPort);
static uint32_t sliceExtract_adjust(Byte *datacube,
                                    Byte *slice,
                                    struct viewPort *viewPort);
static uint32_t vpGenerateTexture(struct vpListElement *currentVp, struct viewerState *viewerState);
static uint32_t downsampleVPTexture(struct viewPort *viewPort);
static uint32_t upsampleVPTexture(struct viewPort *viewPort);

//Calculates the upper left pixel of the texture of an orthogonal slice, dependent on viewerState->currentPosition
static uint32_t calcLeftUpperTexAbsPx();

//Initializes the viewer, is called only once after the viewing thread started
static int32_t initViewer();

static int32_t texIndex(uint32_t x,
                        uint32_t y,
                        uint32_t colorMultiplicationFactor,
                        struct viewPortTexture *texture);
static SDL_Cursor *GenCursor(char *xpm[], int xHot, int yHot);
