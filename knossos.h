//static uint32_t isPathString(char *string);
//static uint32_t printUsage();
static int32_t initStates();
static int32_t printConfigValues(struct stateInfo *state);
static uint32_t cleanUpMain(struct stateInfo *state);
static int32_t tempConfigDefaults();
static struct stateInfo *emptyState();
static int32_t readDataConfAndLocalConf(struct stateInfo *state);
static int32_t stripNewlines(char *string);
static int32_t configFromCli(struct stateInfo *state, int argCount, char *arguments[]);
static int32_t loadNeutralLUT(GLuint *lut);
int32_t readConfigFile(char *path, struct stateInfo *state);
#ifdef LINUX
static int32_t catchSegfault(int signum);
#endif


/*
2. Funktion zum durch-switchen von nodes ohne comment, auch branchpoints ohne comment sollen dabei gefunden werden (für offizielle Version nicht nötig)
1. Menu schließen bei klick irgendwo rein
2. cursor wechsel resize
3. allg. cursor Fadenkreuz

*/
