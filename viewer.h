static struct vpList *vpListNew();
static int32_t vpListAddElement(struct vpList *vpList, struct viewPort *viewPort, struct vpBacklog *backlog);
static struct vpList *vpListGenerate(struct viewerState *viewerState);
static int32_t vpListDelElement(struct vpList *list, struct vpListElement *element);
static int32_t vpListDel(struct vpList *list);
static struct vpBacklog *backlogNew();
static int32_t backlogDelElement(struct vpBacklog *backlog, struct vpBacklogElement *element);
static int32_t backlogAddElement(struct vpBacklog *backlog, Coordinate datacube, uint32_t dcOffset, Byte *slice, uint32_t x_px, uint32_t y_px, uint32_t cubeType);
static int32_t backlogDel(struct vpBacklog *backlog);
static int32_t vpHandleBacklog(struct vpListElement *currentVp, struct viewerState *viewerState, struct stateInfo *state);
static uint32_t dcSliceExtract(Byte *datacube,
                               Byte *slice,
                               size_t dcOffset,
                               struct viewPort *viewPort,
                               struct stateInfo *state);
static uint32_t sliceExtract_standard(Byte *datacube,
                                      Byte *slice,
                                      struct viewPort *viewPort,
                                      struct stateInfo *state);
static uint32_t sliceExtract_adjust(Byte *datacube,
                                    Byte *slice,
                                    struct viewPort *viewPort,
                                    struct stateInfo *state);
static uint32_t vpGenerateTexture(struct vpListElement *currentVp, struct viewerState *viewerState, struct stateInfo *state);

//Calculates the upper left pixel of the texture of an orthogonal slice, dependent on viewerState->currentPosition
static uint32_t calcLeftUpperTexAbsPx(struct stateInfo *state);

//Initializes the viewer, is called only once after the viewing thread started
static int32_t initViewer(struct stateInfo *state);

static int32_t texIndex(uint32_t x,
                        uint32_t y,
                        uint32_t colorMultiplicationFactor,
                        struct viewPortTexture *texture,
                        struct stateInfo *state);
static SDL_Cursor *GenCursor(char *xpm[], int xHot, int yHot);
