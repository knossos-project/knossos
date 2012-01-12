static int32_t clientRun(struct stateInfo *state);
static int32_t connectToServer(struct stateInfo *state);
static int32_t closeConnection(struct stateInfo *state);
static float bytesToFloat(Byte *source);
static uint32_t parseInBuffer(struct stateInfo *state);
static uint32_t flushOutBuffer(struct stateInfo *state);
static int32_t cleanUpClient(struct stateInfo *state);
