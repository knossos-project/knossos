static uint32_t DcoiFromPos(Hashtable *Dcoi, struct stateInfo *state);
static CubeSlot *slotListGetElement(CubeSlotList *slotList);
static uint32_t loadCube(Coordinate coordinate, Byte *freeDcSlot, Byte *freeOcSlot, struct stateInfo *state);
static uint32_t cleanUpLoader(struct loaderState *loaderState);

// ALWAYS give the slotList*Element functions an element that
//      1) really is in slotList.
//      2) really exists.
// The functions don't check for that and things will break if it's not the case.
static int32_t slotListDelElement(CubeSlotList *slotList, CubeSlot *element);
static int32_t slotListAddElement(CubeSlotList *slotList, Byte *datacube);
static CubeSlotList *slotListNew();
static int32_t slotListDel(CubeSlotList *delList);
static int32_t initLoader(struct stateInfo *state);
static uint32_t removeLoadedCubes(struct stateInfo *state);
static uint32_t loadCubes(struct stateInfo *state);
static int32_t addCubicDcSet(int32_t x, int32_t y, int32_t z, int32_t edgeLen, Hashtable *target);
